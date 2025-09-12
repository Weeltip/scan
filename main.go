package main

import (
        "encoding/binary"
        "fmt"
        "math/rand"
        "net"
        "os"
        "runtime"
        "sync/atomic"
        "syscall"
        "time"
)

var (
        totalScanned int64
        totalFound   int64
        startTime    time.Time
)

// TCP заголовок для raw пакетов
type TCPHeader struct {
        SrcPort  uint16
        DstPort  uint16
        SeqNum   uint32
        AckNum   uint32
        DataOff  uint8 // 4 bits
        Flags    uint8
        Window   uint16
        Checksum uint16
        Urgent   uint16
}

// IP заголовок 
type IPHeader struct {
        VersionIHL uint8
        ToS        uint8
        Length     uint16
        ID         uint16
        FlagsFragOff uint16
        TTL        uint8
        Protocol   uint8
        Checksum   uint16
        SrcIP      uint32
        DstIP      uint32
}

// Pseudo header для TCP checksum
type PseudoHeader struct {
        SrcIP    uint32
        DstIP    uint32
        Reserved uint8
        Protocol uint8
        TCPLen   uint16
}

// Расчет checksum
func checksum(data []byte) uint16 {
        var sum uint32
        
        // Обработка 16-битных слов
        for i := 0; i < len(data)-1; i += 2 {
                sum += uint32(data[i])<<8 + uint32(data[i+1])
        }
        
        // Обработка последнего байта если есть
        if len(data)%2 == 1 {
                sum += uint32(data[len(data)-1]) << 8
        }
        
        // Складываем переносы
        for sum>>16 > 0 {
                sum = (sum & 0xFFFF) + (sum >> 16)
        }
        
        return uint16(^sum)
}

// Генерация случайного IP
func generateRandomIP() string {
        for {
                a := byte(rand.Intn(254) + 1)
                b := byte(rand.Intn(256))
                c := byte(rand.Intn(256))
                d := byte(rand.Intn(254) + 1)

                // Пропускаем приватные диапазоны
                if !(a == 10 ||
                        (a == 172 && b >= 16 && b <= 31) ||
                        (a == 192 && b == 168) ||
                        a == 127 || a == 0 ||
                        a == 169 || // Link-local
                        a >= 224) { // Multicast
                        return fmt.Sprintf("%d.%d.%d.%d", a, b, c, d)
                }
        }
}

// IP string в uint32
func ipToUint32(ip string) uint32 {
        parts := make([]byte, 4)
        fmt.Sscanf(ip, "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3])
        return uint32(parts[0])<<24 | uint32(parts[1])<<16 | uint32(parts[2])<<8 | uint32(parts[3])
}

// Создание SYN пакета
func createSYNPacket(srcIP, dstIP string, srcPort, dstPort uint16) []byte {
        srcIPUint := ipToUint32(srcIP)
        dstIPUint := ipToUint32(dstIP)
        
        // IP заголовок
        ipHdr := IPHeader{
                VersionIHL:   0x45, // IPv4, 20 bytes header
                ToS:          0,
                Length:       40, // IP header (20) + TCP header (20)
                ID:           uint16(rand.Intn(65535)),
                FlagsFragOff: 0x4000, // Don't Fragment
                TTL:          64,
                Protocol:     6, // TCP
                Checksum:     0, // Заполним позже
                SrcIP:        srcIPUint,
                DstIP:        dstIPUint,
        }
        
        // TCP заголовок
        tcpHdr := TCPHeader{
                SrcPort:  srcPort,
                DstPort:  dstPort,
                SeqNum:   uint32(rand.Intn(4294967295)),
                AckNum:   0,
                DataOff:  0x50, // 5 * 4 = 20 bytes, no options
                Flags:    0x02, // SYN flag
                Window:   8192,
                Checksum: 0, // Заполним позже
                Urgent:   0,
        }
        
        // Преобразуем в байты
        packet := make([]byte, 40)
        
        // IP header
        packet[0] = ipHdr.VersionIHL
        packet[1] = ipHdr.ToS
        binary.BigEndian.PutUint16(packet[2:4], ipHdr.Length)
        binary.BigEndian.PutUint16(packet[4:6], ipHdr.ID)
        binary.BigEndian.PutUint16(packet[6:8], ipHdr.FlagsFragOff)
        packet[8] = ipHdr.TTL
        packet[9] = ipHdr.Protocol
        // Checksum пока 0
        binary.BigEndian.PutUint32(packet[12:16], ipHdr.SrcIP)
        binary.BigEndian.PutUint32(packet[16:20], ipHdr.DstIP)
        
        // TCP header
        binary.BigEndian.PutUint16(packet[20:22], tcpHdr.SrcPort)
        binary.BigEndian.PutUint16(packet[22:24], tcpHdr.DstPort)
        binary.BigEndian.PutUint32(packet[24:28], tcpHdr.SeqNum)
        binary.BigEndian.PutUint32(packet[28:32], tcpHdr.AckNum)
        packet[32] = tcpHdr.DataOff
        packet[33] = tcpHdr.Flags
        binary.BigEndian.PutUint16(packet[34:36], tcpHdr.Window)
        // Checksum пока 0
        binary.BigEndian.PutUint16(packet[38:40], tcpHdr.Urgent)
        
        // Рассчитываем IP checksum
        ipChecksum := checksum(packet[:20])
        binary.BigEndian.PutUint16(packet[10:12], ipChecksum)
        
        // Рассчитываем TCP checksum с pseudo header
        pseudoHdr := make([]byte, 12)
        binary.BigEndian.PutUint32(pseudoHdr[0:4], srcIPUint)
        binary.BigEndian.PutUint32(pseudoHdr[4:8], dstIPUint)
        pseudoHdr[8] = 0 // Reserved
        pseudoHdr[9] = 6 // TCP protocol
        binary.BigEndian.PutUint16(pseudoHdr[10:12], 20) // TCP header length
        
        tcpChecksum := checksum(append(pseudoHdr, packet[20:]...))
        binary.BigEndian.PutUint16(packet[36:38], tcpChecksum)
        
        return packet
}

// Raw SYN сканирование
func rawSynScan(dstIP string, port uint16, socket int) {
        atomic.AddInt64(&totalScanned, 1)
        
        // Случайный source port
        srcPort := uint16(rand.Intn(32767) + 32768)
        srcIP := "192.168.1.100" // Фейковый source IP
        
        // Создаем SYN пакет
        packet := createSYNPacket(srcIP, dstIP, srcPort, port)
        
        // Отправляем пакет
        dstAddr := &syscall.SockaddrInet4{Port: int(port)}
        copy(dstAddr.Addr[:], net.ParseIP(dstIP).To4())
        
        err := syscall.Sendto(socket, packet, 0, dstAddr)
        if err == nil {
                // В реальном SYN scan нужно слушать SYN-ACK ответы
                // Здесь просто считаем что отправили пакет
                atomic.AddInt64(&totalFound, 1)
                fmt.Println(dstIP) // Выводим как найденный
        }
}

// Worker для raw сканирования
func rawScanWorker(ipChan <-chan string, port uint16, socket int) {
        for ip := range ipChan {
                rawSynScan(ip, port, socket)
        }
}

// Статистика
func printStats() {
        ticker := time.NewTicker(5 * time.Second)
        defer ticker.Stop()

        for range ticker.C {
                elapsed := time.Since(startTime)
                scanned := atomic.LoadInt64(&totalScanned)
                found := atomic.LoadInt64(&totalFound)
                rate := float64(scanned) / elapsed.Seconds()

                fmt.Fprintf(os.Stderr, "[INFO] raw_syn_scanner: sent %d SYN packets (%d hosts, %.0f/sec)\n", 
                        scanned, found, rate)
        }
}

func main() {
        if len(os.Args) < 3 {
                fmt.Fprintf(os.Stderr, "Usage: %s <port> <threads> [rate_limit]\n", os.Args[0])
                fmt.Fprintf(os.Stderr, "Example: %s 23 1000 5000\n", os.Args[0])
                fmt.Fprintf(os.Stderr, "\nRaw SYN packet scanner (zmap style)\n")
                fmt.Fprintf(os.Stderr, "Requires root privileges for raw socket access\n")
                fmt.Fprintf(os.Stderr, "Output: Target IP addresses (one per line)\n")
                os.Exit(1)
        }

        port := 23
        threads := 1000
        rateLimit := 5000

        fmt.Sscanf(os.Args[1], "%d", &port)
        fmt.Sscanf(os.Args[2], "%d", &threads)
        
        if len(os.Args) > 3 {
                fmt.Sscanf(os.Args[3], "%d", &rateLimit)
        }

        // Создаем raw socket
        socket, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_RAW, syscall.IPPROTO_TCP)
        if err != nil {
                fmt.Fprintf(os.Stderr, "[ERROR] Failed to create raw socket: %v\n", err)
                fmt.Fprintf(os.Stderr, "[ERROR] This scanner requires root privileges\n")
                fmt.Fprintf(os.Stderr, "[ERROR] Run with: sudo %s %s\n", os.Args[0], os.Args[1])
                os.Exit(1)
        }
        defer syscall.Close(socket)

        // Включаем IP_HDRINCL для включения IP заголовка
        err = syscall.SetsockoptInt(socket, syscall.IPPROTO_IP, syscall.IP_HDRINCL, 1)
        if err != nil {
                fmt.Fprintf(os.Stderr, "[ERROR] Failed to set IP_HDRINCL: %v\n", err)
                os.Exit(1)
        }

        // Инициализация
        runtime.GOMAXPROCS(runtime.NumCPU())
        rand.Seed(time.Now().UnixNano())
        startTime = time.Now()

        fmt.Fprintf(os.Stderr, "[INFO] raw_syn_scanner: starting raw SYN scan\n")
        fmt.Fprintf(os.Stderr, "[INFO] raw_syn_scanner: target port %d, %d threads, %d rate limit\n", 
                port, threads, rateLimit)
        fmt.Fprintf(os.Stderr, "[INFO] raw_syn_scanner: sending raw SYN packets to random IPv4 space\n")
        fmt.Fprintf(os.Stderr, "[INFO] raw_syn_scanner: target IPs will be printed to stdout\n")

        // Каналы
        ipChan := make(chan string, threads*10)

        // Статистика
        go printStats()

        // Запуск воркеров
        for i := 0; i < threads; i++ {
                go rawScanWorker(ipChan, uint16(port), socket)
        }

        // Rate limiting
        rateTicker := time.NewTicker(time.Second / time.Duration(rateLimit))
        defer rateTicker.Stop()

        // Генерация IP с rate limiting
        for {
                select {
                case <-rateTicker.C:
                        select {
                        case ipChan <- generateRandomIP():
                        default:
                                // Канал полный, пропускаем
                        }
                }
        }
}
