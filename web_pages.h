#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

const char HTML_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-C3 Matrix Clock Control</title>
  <style>
    :root {
      --bg-color: #0f172a;
      --card-bg: #1e293b;
      --accent-color: #38bdf8;
      --accent-hover: #0284c7;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --border-color: #334155;
      --success-color: #4ade80;
      --warning-color: #fbbf24;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }
    body { background-color: var(--bg-color); color: var(--text-main); padding: 20px; display: flex; justify-content: center; min-height: 100vh; }
    .container { max-width: 600px; width: 100%; display: flex; flex-direction: column; gap: 20px; }
    .header { text-align: center; padding: 15px 0; border-bottom: 1px solid var(--border-color); }
    .header h1 { font-size: 1.8rem; color: var(--accent-color); margin-bottom: 5px; }
    .header p { color: var(--text-muted); font-size: 0.9rem; }
    .card { background-color: var(--card-bg); border-radius: 12px; padding: 20px; border: 1px solid var(--border-color); box-shadow: 0 4px 6px -1px rgba(0,0,0,0.3); }
    .card-title { font-size: 1.2rem; font-weight: 600; color: var(--accent-color); margin-bottom: 15px; display: flex; align-items: center; justify-content: space-between; }
    .live-time-box { text-align: center; font-size: 2.5rem; font-weight: 700; font-family: monospace; letter-spacing: 2px; color: var(--success-color); padding: 10px; background: rgba(0,0,0,0.3); border-radius: 8px; border: 1px solid var(--border-color); }
    .form-group { margin-bottom: 15px; }
    label { display: block; font-size: 0.9rem; color: var(--text-muted); margin-bottom: 6px; font-weight: 500; }
    input[type="text"], input[type="number"], select { width: 100%; padding: 10px 12px; background: #0f172a; border: 1px solid var(--border-color); border-radius: 6px; color: var(--text-main); font-size: 1rem; outline: none; transition: border 0.2s; }
    input[type="text"]:focus, select:focus { border-color: var(--accent-color); }
    .slider-container { display: flex; align-items: center; gap: 15px; }
    input[type="range"] { flex: 1; accent-color: var(--accent-color); cursor: pointer; }
    .slider-val { font-weight: bold; width: 30px; text-align: center; color: var(--accent-color); }
    .toggle-group { display: flex; align-items: center; justify-content: space-between; padding: 8px 0; }
    .switch { position: relative; display: inline-block; width: 48px; height: 24px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #334155; transition: .3s; border-radius: 24px; }
    .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .3s; border-radius: 50%; }
    input:checked + .slider { background-color: var(--accent-color); }
    input:checked + .slider:before { transform: translateX(24px); }
    .btn { display: inline-block; width: 100%; padding: 12px; background: var(--accent-color); color: #0f172a; font-weight: 700; border: none; border-radius: 6px; font-size: 1rem; cursor: pointer; transition: background 0.2s, transform 0.1s; text-align: center; text-decoration: none; }
    .btn:hover { background: var(--accent-hover); color: white; }
    .btn:active { transform: scale(0.98); }
    .btn-secondary { background: transparent; border: 1px solid var(--border-color); color: var(--text-main); margin-top: 10px; }
    .btn-secondary:hover { background: #334155; color: white; }
    .btn-danger { background: #ef4444; color: white; margin-top: 10px; }
    .btn-danger:hover { background: #dc2626; }
    .status-badge { display: inline-block; padding: 3px 8px; border-radius: 12px; font-size: 0.75rem; font-weight: bold; }
    .badge-online { background: rgba(74, 222, 128, 0.2); color: var(--success-color); border: 1px solid var(--success-color); }
    .toast { visibility: hidden; min-width: 250px; background-color: var(--accent-color); color: #0f172a; font-weight: bold; text-align: center; border-radius: 6px; padding: 12px; position: fixed; z-index: 10; bottom: 30px; left: 50%; transform: translateX(-50%); box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
    .toast.show { visibility: visible; animation: fadein 0.5s, fadeout 0.5s 2.5s; }
    @keyframes fadein { from { bottom: 0; opacity: 0; } to { bottom: 30px; opacity: 1; } }
    @keyframes fadeout { from { bottom: 30px; opacity: 1; } to { bottom: 0; opacity: 0; } }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>ESP32-C3 Matrix Clock</h1>
      <p>MAX7219 4-in-1 Dot Matrix Display Control</p>
    </div>

    <!-- Live Time Box -->
    <div class="card">
      <div class="card-title">
        <span>Current Time</span>
        <span class="status-badge badge-online" id="wifi-status">Connected</span>
      </div>
      <div class="live-time-box" id="clock-display">00:00:00</div>
      <div style="text-align:center; margin-top: 8px; font-size: 0.85rem; color: var(--text-muted);" id="date-display">
        Loading date...
      </div>
    </div>

    <!-- Clock & Display Settings -->
    <div class="card">
      <div class="card-title">Display & Time Configuration</div>
      <form id="settings-form">
        
        <div class="form-group">
          <label for="brightness">Display Brightness (0 - 15)</label>
          <div class="slider-container">
            <input type="range" id="brightness" name="brightness" min="0" max="15" value="3" oninput="updateBrightness(this.value)">
            <span class="slider-val" id="brightness-val">3</span>
          </div>
        </div>

        <div class="form-group">
          <div class="toggle-group">
            <span>24-Hour Time Format</span>
            <label class="switch">
              <input type="checkbox" id="is24Hour" name="is24Hour" checked>
              <span class="slider"></span>
            </label>
          </div>
        </div>

        <div class="form-group">
          <div class="toggle-group">
            <span>Scroll Date Periodically</span>
            <label class="switch">
              <input type="checkbox" id="showDateInterval" name="showDateInterval" checked>
              <span class="slider"></span>
            </label>
          </div>
        </div>

        <div class="form-group">
          <label for="timezone">Timezone (POSIX Format)</label>
          <select id="timezone-select" onchange="setTimezonePreset(this.value)" style="margin-bottom: 8px;">
            <option value="">-- Select Preset --</option>
            <option value="IST-5:30">India (IST)</option>
            <option value="EST5EDT,M3.2.0,M11.1.0">US Eastern (EST/EDT)</option>
            <option value="CST6CDT,M3.2.0,M11.1.0">US Central (CST/CDT)</option>
            <option value="PST8PDT,M3.2.0,M11.1.0">US Pacific (PST/PDT)</option>
            <option value="GMT0BST,M3.5.0/1,M10.5.0">United Kingdom (GMT/BST)</option>
            <option value="CET-1CEST,M3.5.0,M10.5.0/3">Central Europe (CET/CEST)</option>
            <option value="JST-9">Japan / Korea (JST/KST)</option>
            <option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia Eastern (AEST/AEDT)</option>
            <option value="UTC0">UTC</option>
          </select>
          <input type="text" id="timezone" name="timezone" placeholder="e.g. IST-5:30 or EST5EDT..." required>
        </div>

        <div class="form-group">
          <label for="ntpServer">NTP Time Server</label>
          <input type="text" id="ntpServer" name="ntpServer" value="pool.ntp.org" required>
        </div>

        <button type="button" class="btn" onclick="saveSettings()">Save Settings</button>
        <button type="button" class="btn btn-secondary" onclick="syncNtp()">Force NTP Sync Now</button>
      </form>
    </div>

    <!-- Network Actions -->
    <div class="card">
      <div class="card-title">Network & System</div>
      <p style="font-size:0.85rem; color: var(--text-muted); margin-bottom: 12px;">
        mDNS Hostname: <strong>http://clock.local</strong><br>
        IP Address: <span id="ip-addr">Loading...</span>
      </p>
      <button type="button" class="btn btn-secondary" onclick="restartDevice()">Restart Clock</button>
      <button type="button" class="btn btn-danger" onclick="resetWifi()">Change Wi-Fi Network</button>
    </div>
  </div>

  <div id="toast" class="toast">Settings Saved!</div>

  <script>
    function showToast(msg) {
      const toast = document.getElementById('toast');
      toast.innerText = msg;
      toast.className = 'toast show';
      setTimeout(() => { toast.className = toast.className.replace('toast show', 'toast'); }, 3000);
    }

    function setTimezonePreset(val) {
      if (val) {
        document.getElementById('timezone').value = val;
      }
    }

    function updateBrightness(val) {
      document.getElementById('brightness-val').innerText = val;
      // Live brightness preview trigger
      fetch('/api/brightness?val=' + val, { method: 'POST' }).catch(e => console.log(e));
    }

    function fetchStatus() {
      fetch('/api/status')
        .then(res => res.json())
        .then(data => {
          document.getElementById('clock-display').innerText = data.timeStr || '--:--:--';
          document.getElementById('date-display').innerText = data.dateStr || '';
          document.getElementById('ip-addr').innerText = data.ip || 'Unknown';
        })
        .catch(err => console.error('Status fetch error:', err));
    }

    function loadSettings() {
      fetch('/api/settings')
        .then(res => res.json())
        .then(data => {
          document.getElementById('brightness').value = data.brightness;
          document.getElementById('brightness-val').innerText = data.brightness;
          document.getElementById('is24Hour').checked = data.is24Hour;
          document.getElementById('showDateInterval').checked = data.showDateInterval;
          document.getElementById('timezone').value = data.timezone;
          document.getElementById('ntpServer').value = data.ntpServer;
        })
        .catch(err => console.error('Settings load error:', err));
    }

    function saveSettings() {
      const payload = {
        brightness: parseInt(document.getElementById('brightness').value),
        is24Hour: document.getElementById('is24Hour').checked,
        showDateInterval: document.getElementById('showDateInterval').checked,
        timezone: document.getElementById('timezone').value,
        ntpServer: document.getElementById('ntpServer').value
      };

      fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      })
      .then(res => res.json())
      .then(res => {
        showToast('Settings Saved Successfully!');
      })
      .catch(err => {
        showToast('Failed to save settings');
      });
    }

    function syncNtp() {
      fetch('/api/sync', { method: 'POST' })
        .then(res => res.json())
        .then(data => showToast('NTP Sync Triggered!'))
        .catch(err => showToast('Sync Failed'));
    }

    function restartDevice() {
      if (confirm('Are you sure you want to restart the ESP32 clock?')) {
        fetch('/api/restart', { method: 'POST' });
        showToast('Restarting device...');
      }
    }

    function resetWifi() {
      if (confirm('This will wipe saved WiFi credentials and start Access Point mode! Continue?')) {
        fetch('/api/reset_wifi', { method: 'POST' });
        showToast('WiFi Reset! Rebooting to AP Mode...');
      }
    }

    // Initial load & timers
    loadSettings();
    fetchStatus();
    setInterval(fetchStatus, 1000);
  </script>
</body>
</html>
)rawliteral";

const char HTML_WIFI_SETUP[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-C3 Wi-Fi Setup</title>
  <style>
    :root {
      --bg-color: #0f172a;
      --card-bg: #1e293b;
      --accent-color: #38bdf8;
      --accent-hover: #0284c7;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --border-color: #334155;
      --success-color: #4ade80;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }
    body { background-color: var(--bg-color); color: var(--text-main); padding: 20px; display: flex; justify-content: center; min-height: 100vh; }
    .container { max-width: 450px; width: 100%; display: flex; flex-direction: column; gap: 20px; }
    .header { text-align: center; padding: 15px 0; border-bottom: 1px solid var(--border-color); }
    .header h1 { font-size: 1.6rem; color: var(--accent-color); margin-bottom: 5px; }
    .header p { color: var(--text-muted); font-size: 0.9rem; }
    .card { background-color: var(--card-bg); border-radius: 12px; padding: 20px; border: 1px solid var(--border-color); box-shadow: 0 4px 6px -1px rgba(0,0,0,0.3); }
    .form-group { margin-bottom: 15px; }
    label { display: block; font-size: 0.9rem; color: var(--text-muted); margin-bottom: 6px; font-weight: 500; }
    input[type="text"], input[type="password"], select { width: 100%; padding: 10px 12px; background: #0f172a; border: 1px solid var(--border-color); border-radius: 6px; color: var(--text-main); font-size: 1rem; outline: none; transition: border 0.2s; }
    input[type="text"]:focus, input[type="password"]:focus, select:focus { border-color: var(--accent-color); }
    .btn { display: inline-block; width: 100%; padding: 12px; background: var(--accent-color); color: #0f172a; font-weight: 700; border: none; border-radius: 6px; font-size: 1rem; cursor: pointer; transition: background 0.2s, transform 0.1s; text-align: center; }
    .btn:hover { background: var(--accent-hover); color: white; }
    .btn-secondary { background: transparent; border: 1px solid var(--border-color); color: var(--text-main); margin-top: 8px; }
    .btn-secondary:hover { background: #334155; }
    .status-msg { text-align: center; margin-top: 15px; font-weight: 500; font-size: 0.95rem; color: var(--success-color); }
    .scan-loader { text-align: center; color: var(--accent-color); font-size: 0.85rem; margin-top: 4px; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>ESP32 Clock Wi-Fi Setup</h1>
      <p>Connect your ESP32-C3 Matrix Clock to Wi-Fi</p>
    </div>

    <div class="card">
      <form id="wifi-form">
        <div class="form-group">
          <label for="ssid-select">Select 2.4GHz Wi-Fi Network</label>
          <select id="ssid-select" onchange="onSsidSelect(this.value)">
            <option value="">-- Scanning networks... --</option>
          </select>
          <div class="scan-loader" id="scan-status">Scanning nearby Wi-Fi networks...</div>
        </div>

        <div class="form-group">
          <label for="ssid">Wi-Fi Name (SSID)</label>
          <input type="text" id="ssid" name="ssid" placeholder="Enter or select SSID..." required>
        </div>

        <div class="form-group">
          <label for="password">Wi-Fi Password</label>
          <input type="password" id="password" name="password" placeholder="Enter Wi-Fi Password">
          <div style="margin-top: 6px; font-size: 0.85rem;">
            <label style="display:inline; cursor:pointer;">
              <input type="checkbox" onclick="togglePass()"> Show Password
            </label>
          </div>
        </div>

        <button type="button" class="btn" onclick="saveWifi()">Save & Connect</button>
        <button type="button" class="btn btn-secondary" onclick="scanWifi()">Rescan Networks</button>
      </form>
      <div id="status" class="status-msg"></div>
    </div>
  </div>

  <script>
    function togglePass() {
      const p = document.getElementById('password');
      p.type = (p.type === 'password') ? 'text' : 'password';
    }

    function onSsidSelect(val) {
      if (val) {
        document.getElementById('ssid').value = val;
      }
    }

    function scanWifi() {
      const select = document.getElementById('ssid-select');
      const status = document.getElementById('scan-status');
      select.innerHTML = '<option value="">-- Scanning networks... --</option>';
      status.innerText = 'Scanning nearby 2.4GHz Wi-Fi networks...';

      fetch('/api/wifi_scan')
        .then(res => res.json())
        .then(networks => {
          select.innerHTML = '<option value="">-- Select from scanned networks --</option>';
          if (!networks || networks.length === 0) {
            status.innerText = 'No 2.4GHz networks found. Type SSID manually below.';
            return;
          }
          status.innerText = `Found ${networks.length} network(s).`;
          networks.forEach(net => {
            const opt = document.createElement('option');
            opt.value = net.ssid;
            opt.innerText = `${net.ssid} (${net.rssi} dBm, Ch:${net.channel})`;
            select.appendChild(opt);
          });
        })
        .catch(err => {
          status.innerText = 'Scan failed. Enter SSID manually below.';
          select.innerHTML = '<option value="">-- Scan failed --</option>';
        });
    }

    function saveWifi() {
      const ssid = document.getElementById('ssid').value.trim();
      const password = document.getElementById('password').value;
      const status = document.getElementById('status');

      if (!ssid) {
        alert('Please enter or select a Wi-Fi SSID!');
        return;
      }

      status.innerText = 'Saving credentials and restarting clock...';

      fetch('/api/save_wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid: ssid, password: password })
      })
      .then(res => res.json())
      .then(data => {
        status.innerText = 'Saved successfully! Clock is rebooting to connect...';
      })
      .catch(err => {
        status.innerText = 'Saved! Rebooting device...';
      });
    }

    // Auto-scan on load
    scanWifi();
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGES_H
