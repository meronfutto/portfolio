# SOC Alert Triage Log

## Alert #1

**Date:** 2025-01-22 02:37:00 +03:00
**Alert Title:** SOC335 - CVE-2024-49138 Exploitation Detected
**Severity:** Medium
**Event ID:** 313
**Type:** Privilege Escalation

### Investigation
- Alert triggered for svohost.exe.
- Parent process: powershell.exe.
- Process path: C:\temp\service_installer\svohost.exe.
- Checked the SHA-256 hash in Threat Intelligence and VirusTotal.
- The file was identified as malicious and associated with CVE-2024-49138.
- Identified multiple failed RDP login attempts from 185.107.56.141 to host Victor (172.16.17.207), followed by a successful login.
- PowerShell was executed under the Victor user account.
- whoami /priv and whoami were executed for privilege and user discovery.
- PowerShell downloaded service-installer.zip, extracted it, and executed svohost.exe.
- After svohost.exe execution, a child PowerShell process was observed running as NT AUTHORITY\SYSTEM, indicating successful privilege escalation.

### Evidence
- Attacker IP: 185.107.56.141
- Victim host: Victor (172.16.17.207)
- Malicious file: C:\temp\service_installer\svohost.exe
- SHA-256: b432dcf4a0f0b601b1d79848467137a5e25cab5a0b7b1224be9d3b6540122db9
- MD5: 0e66fce898aceb3f3f84e3b8a09d3367
- Download URL: https://files-ld.s3.us-east-2.amazonaws.com/service-installer.zip
- Privilege escalation observed: Victor → NT AUTHORITY\SYSTEM

### Classification
- True Positive – Malicious

### Action Taken
- Added identified IOCs to the case.
- Contained the affected endpoint Victor using EDR.
- Completed the incident response playbook.

### Time to Triage
- Approximately 3–4 hours