/**
 * @file index_html.h
 * @brief Embedded HTML configuration interface
 * @details Contains complete HTML/CSS/JavaScript for web configuration portal.
 *          Provides UI for:
 *          - Viewing detected TPMS sensors in real-time
 *          - Configuring sensor addresses (front/rear)
 *          - Setting ideal tire pressures (PSI)
 *          - Selecting pressure unit (PSI/BAR)
 *          - Clearing configuration
 *          - Device restart
 *          
 *          Served by WebServer::handleRoot() at GET /
 *          Uses REST API endpoints:
 *          - GET /api/sensors - Refresh sensor list
 *          - GET /api/config - Load current config
 *          - POST /api/config - Save configuration
 *          - POST /api/clear - Clear sensor pairing
 *          - POST /api/restart - Reboot device
 *          
 *          Design: Dark theme, responsive, motorcycle-themed (🏍️ icon)
 */

#pragma once

static const char *index_html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BTPMS Config</title>
    <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='75' font-size='80'>🏍️</text></svg>">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: #1a1a1a;
            color: #ffffff;
            padding: 20px;
            line-height: 1.6;
        }
        .container { max-width: 600px; margin: 0 auto; }
        h1 { color: #4CAF50; margin-bottom: 30px; text-align: center; }
        h2 { color: #8BC34A; margin: 25px 0 15px 0; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }
        .card { 
            background: #2d2d2d; 
            border-radius: 8px; 
            padding: 20px; 
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        }
        .sensor { 
            padding: 15px; 
            margin: 10px 0; 
            background: #3d3d3d; 
            border-radius: 6px;
            border-left: 4px solid #4CAF50;
        }
        .sensor-info { display: flex; justify-content: space-between; margin: 5px 0; }
        .label { color: #aaa; }
        .value { color: #fff; font-weight: 600; }
        input, button { 
            width: 100%; 
            padding: 12px; 
            margin: 10px 0; 
            border-radius: 6px; 
            border: 1px solid #555;
            font-size: 16px;
        }
        input { 
            background: #3d3d3d; 
            color: #fff;
        }
        input:focus {
            outline: none;
            border-color: #4CAF50;
        }
        button { 
            background: #4CAF50; 
            color: white; 
            border: none; 
            cursor: pointer;
            font-weight: 600;
            transition: background 0.3s;
        }
        button:hover { background: #45a049; }
        button:disabled { 
            background: #555; 
            cursor: not-allowed;
        }
        .btn-secondary { background: #2196F3; }
        .btn-secondary:hover { background: #0b7dda; }
        .btn-danger { background: #f44336; }
        .btn-danger:hover { background: #da190b; }
        .status { 
            padding: 10px; 
            border-radius: 6px; 
            margin: 10px 0;
            text-align: center;
            font-weight: 600;
        }
        .status.success { background: #4CAF50; }
        .status.error { background: #f44336; }
        .status.info { background: #2196F3; }
        #loading { display: none; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🏍️ BTPMS Config</h1>
        
        <div class="card">
            <h2>📡 Discovered Sensors</h2>
            <div id="sensors">Loading sensors...</div>
            <button onclick="refreshSensors()" class="btn-secondary">Refresh</button>
        </div>

        <div class="card">
            <h2>⚙️ Configuration</h2>
            <label class="label">Front Wheel Address:</label>
            <input type="text" id="frontAddr" placeholder="00:00:00:00:00:00">
            
            <label class="label">Rear Wheel Address:</label>
            <input type="text" id="rearAddr" placeholder="00:00:00:00:00:00">
            
            <label class="label">Front Ideal PSI:</label>
            <input type="number" id="frontPsi" step="0.1" min="0" max="100">
            
            <label class="label">Rear Ideal PSI:</label>
            <input type="number" id="rearPsi" step="0.1" min="0" max="100">
            
            <label class="label">Pressure Unit:</label>
            <select id="pressureUnit" style="width: 100%; padding: 12px; margin: 10px 0; border-radius: 6px; border: 1px solid #555; font-size: 16px; background: #3d3d3d; color: #fff;">
                <option value="PSI">PSI</option>
                <option value="BAR">BAR</option>
            </select>
            
            <button onclick="saveConfig()">💾 Save Configuration</button>
            <button onclick="clearConfig()" class="btn-danger">🗑️ Clear Configuration</button>
            <button onclick="restartDevice()" class="btn-danger">🔄 Restart Device</button>
        </div>

        <div id="status" class="status" style="display: none;"></div>
        <div id="loading" class="status info">Processing...</div>
    </div>

    <script>
        function showStatus(message, type) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
            setTimeout(() => status.style.display = 'none', 3000);
        }

        function showLoading(show) {
            document.getElementById('loading').style.display = show ? 'block' : 'none';
        }

        async function refreshSensors() {
            showLoading(true);
            try {
                const response = await fetch('/api/sensors');
                const data = await response.json();
                
                const sensorsDiv = document.getElementById('sensors');
                if (data.sensors.length === 0) {
                    sensorsDiv.innerHTML = '<p class="label">No sensors detected</p>';
                } else {
                    sensorsDiv.innerHTML = data.sensors.map(s => `
                        <div class="sensor">
                            <div class="sensor-info">
                                <span class="label">Address:</span>
                                <span class="value">${s.address}</span>
                            </div>
                            <div class="sensor-info">
                                <span class="label">Pressure:</span>
                                <span class="value">${s.pressure} PSI</span>
                            </div>
                            <div class="sensor-info">
                                <span class="label">Temperature:</span>
                                <span class="value">${s.temperature}°C</span>
                            </div>
                            <div class="sensor-info">
                                <span class="label">Battery:</span>
                                <span class="value">${s.battery}%</span>
                            </div>
                            <button onclick="setFront('${s.address}')" class="btn-secondary">Set as Front</button>
                            <button onclick="setRear('${s.address}')" class="btn-secondary">Set as Rear</button>
                        </div>
                    `).join('');
                }
                showStatus('Sensors refreshed', 'success');
            } catch (e) {
                showStatus('Failed to load sensors', 'error');
            }
            showLoading(false);
        }

        async function saveConfig() {
            showLoading(true);
            const config = {
                front_address: document.getElementById('frontAddr').value,
                rear_address: document.getElementById('rearAddr').value,
                front_ideal_psi: parseFloat(document.getElementById('frontPsi').value),
                rear_ideal_psi: parseFloat(document.getElementById('rearPsi').value),
                pressure_unit: document.getElementById('pressureUnit').value
            };

            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });

                if (response.ok) {
                    showStatus('Configuration saved!', 'success');
                } else {
                    showStatus('Failed to save config', 'error');
                }
            } catch (e) {
                showStatus('Failed to save config', 'error');
            }
            showLoading(false);
        }

        function setFront(address) {
            document.getElementById('frontAddr').value = address;
            showStatus('Front address set', 'info');
        }

        function setRear(address) {
            document.getElementById('rearAddr').value = address;
            showStatus('Rear address set', 'info');
        }

        async function loadConfig() {
            try {
                const response = await fetch('/api/config');
                const config = await response.json();
                document.getElementById('frontAddr').value = config.front_address || '';
                document.getElementById('rearAddr').value = config.rear_address || '';
                document.getElementById('frontPsi').value = config.front_ideal_psi || 36;
                document.getElementById('rearPsi').value = config.rear_ideal_psi || 42;
                document.getElementById('pressureUnit').value = config.pressure_unit || 'PSI';
            } catch (e) {
                showStatus('Failed to load config', 'error');
            }
        }

        async function saveConfig() {
            showLoading(true);
            const config = {
                front_address: document.getElementById('frontAddr').value,
                rear_address: document.getElementById('rearAddr').value,
                front_ideal_psi: parseFloat(document.getElementById('frontPsi').value),
                rear_ideal_psi: parseFloat(document.getElementById('rearPsi').value),
                pressure_unit: document.getElementById('pressureUnit').value
            };

            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });

                if (response.ok) {
                    showStatus('Configuration saved!', 'success');
                } else {
                    showStatus('Failed to save config', 'error');
                }
            } catch (e) {
                showStatus('Failed to save config', 'error');
            }
            showLoading(false);
        }

        async function clearConfig() {
            if (!confirm('Clear all configuration? This will reset sensor addresses and ideal PSI values.')) return;
            
            showLoading(true);
            try {
                const response = await fetch('/api/clear', { method: 'POST' });
                if (response.ok) {
                    showStatus('Configuration cleared!', 'success');
                    // Reload config to show defaults
                    setTimeout(() => loadConfig(), 500);
                } else {
                    showStatus('Failed to clear config', 'error');
                }
            } catch (e) {
                showStatus('Failed to clear config', 'error');
            }
            showLoading(false);
        }

        async function restartDevice() {
            if (!confirm('Restart device? This will close the configuration portal.')) return;
            
            showLoading(true);
            try {
                await fetch('/api/restart', { method: 'POST' });
                showStatus('Restarting device...', 'info');
                setTimeout(() => {
                    showStatus('Device restarted. Please reconnect.', 'success');
                }, 2000);
            } catch (e) {
                showStatus('Restart initiated', 'info');
            }
        }

        // Auto-refresh sensors every 5 seconds
        setInterval(refreshSensors, 5000);
        
        // Initial load
        loadConfig();
        refreshSensors();
    </script>
</body>
</html>
)HTML";
