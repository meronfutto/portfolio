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

## Alert #2

**Alert Time:** 2025-07-22 13:07:10 +03:00
**Alert Title:** SOC342 - CVE-2025-53770 SharePoint ToolShell Auth Bypass and RCE
**Severity:** Critical
**Event ID:** 320
**Type:** Web Attack
**Environment:** LetsDefend simulated SOC lab.

### Investigation
- 13:07:10 — The proxy logged an inbound `POST` from `107.191.58.76` to `SharePoint01` (`172.16.20.17:443`). The request targeted `/_layouts/15/ToolPane.aspx?DisplayMode=Edit&a=/ToolPane.aspx`, listed `/_layouts/SignOut.aspx` as the `Referer`, and declared a body size of 7,699 bytes. The device action was `Allowed`. The request body and HTTP response status were not available in the reviewed records.
- 13:07:11 — Endpoint telemetry for IIS worker process `w3wp.exe` (PID 4560) showed a target PowerShell command with a Base64 argument. Decoding the argument revealed ASP.NET/C# markup intended to read and return `machineKey` settings, including validation and decryption keys.
- 13:07:24 — Endpoint telemetry recorded `powershell.exe` (PID 9876) under `IIS APPPOOL\SharePoint - 80`, with arguments to run without loading a profile, hide the window, and accept an encoded command.
- 13:07:27 — Terminal History showed `csc.exe` invoked to compile `C:\Windows\Temp\payload.cs` into `C:\Windows\Temp\payload.exe`. Compilation output and execution of the resulting file were not verified.
- 13:07:29 — `cmd.exe` (PID 9910) ran under the SharePoint application pool account with a command attempting to write `spinstall0.aspx` in SharePoint's `TEMPLATE\LAYOUTS` directory. The recorded markup referenced a redirect to `hxxp://107.191.58[.]76/payload.exe`, using the same external IP as the initial `POST`.
- 13:07:34 — A separate PowerShell process (PID 9920), with `cmd.exe` listed as its parent, attempted to retrieve the `machineKey` configuration. The command output was not visible.

### Evidence
- Source IP: `107.191.58.76`. LetsDefend Threat Intel tagged it `CVE-2025-53770`. At the time of review, VirusTotal showed 8/92 detections and listed AS20473 (The Constant Company, LLC).
- Affected endpoint: `SharePoint01` (`172.16.20.17`).
- Redirect target observed in the write command: `hxxp://107.191.58[.]76/payload.exe`. Download and file hash were not verified.

### Classification
- True Positive – Malicious. The suspicious inbound request and subsequent process activity under the SharePoint application pool account support successful command execution. The redirect target ties that activity to the source IP of the request.
- Creation or execution of `spinstall0.aspx`, execution of `payload.exe`, and disclosure of `machineKey` secrets were not verified.

### Response
- Completed the playbook and submitted the case as True Positive.
- Tier 2 escalation was warranted to verify file creation, review follow-up HTTP requests, and determine whether other hosts were affected. Endpoint isolation status was not verified in the available evidence.

### Supporting Screenshots
- [Alert overview](screenshots/SOC342/01-alert-overview.png)
- [HTTP Host header](screenshots/SOC342/02-http-host-header.png)
- [HTTP Accept header](screenshots/SOC342/03-http-accept-header.png)
- [Proxy source and destination](screenshots/SOC342/04-proxy-connection.png)
- [HTTP request and alert trigger details](screenshots/SOC342/05-http-alert-details.png)
- [IIS process telemetry](screenshots/SOC342/06-iis-target-command.png)
- [Encoded PowerShell argument](screenshots/SOC342/07-encoded-command.png)
- [PowerShell process and account](screenshots/SOC342/08-powershell-process.png)
- [Command attempting to write spinstall0.aspx](screenshots/SOC342/09-aspx-write-command.png)
- [MachineKey configuration command](screenshots/SOC342/10-machinekey-command.png)
- [Terminal history with compilation and redirect commands](screenshots/SOC342/11-terminal-history.png)
- [LetsDefend Threat Intelligence](screenshots/SOC342/12-threat-intel.png)
- [VirusTotal IP reputation and ASN](screenshots/SOC342/13-virustotal.png)
- [True Positive selected before submission](screenshots/SOC342/14-review-and-submit.png)
