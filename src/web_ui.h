#pragma once

// Weboberfläche als eine Seite, ohne externe Abhängigkeiten (funktioniert auch im
// Setup-Netz ohne Internet).

static const char INDEX_HTML[] = R"HTML(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Minecraft Sign</title>
<style>
:root{--bg:#17171b;--panel:#24242a;--edge:#3a3a44;--text:#e9e9ee;--muted:#9a9aa6;--accent:#62b03f;--accent2:#4a8a2e;--warn:#d9534f}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:15px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
header{padding:16px 16px 0;max-width:900px;margin:auto}
h1{margin:0 0 4px;font:700 22px/1.2 ui-monospace,Menlo,Consolas,monospace;letter-spacing:.18em}
h2{font-size:13px;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);margin:0 0 10px}
main{max-width:900px;margin:auto;padding:12px 16px 40px}
.panel{background:var(--panel);border:2px solid var(--edge);border-radius:6px;padding:14px;margin-bottom:14px}
#preview{width:100%;display:block;background:#0c0c0e;border-radius:4px}
.tabs{display:flex;gap:6px;margin:12px 0}
.tabs button{flex:1}
button,.btn{background:#33333c;color:var(--text);border:2px solid var(--edge);border-radius:4px;padding:9px 12px;font:inherit;cursor:pointer}
button:hover{border-color:#55555f}
button.primary,.tabs button.active{background:var(--accent2);border-color:var(--accent)}
button.danger{border-color:var(--warn);color:#ffb3b1}
.row{display:flex;align-items:center;gap:12px;margin:10px 0;flex-wrap:wrap}
.row label{min-width:120px;color:var(--muted)}
.row input[type=range]{flex:1;min-width:140px;accent-color:var(--accent)}
.row output{min-width:36px;text-align:right;font-variant-numeric:tabular-nums}
input[type=text],input[type=password],input[type=number],select{background:#1b1b20;color:var(--text);border:2px solid var(--edge);border-radius:4px;padding:8px;font:inherit;flex:1;min-width:0}
input[type=color]{width:56px;height:36px;border:2px solid var(--edge);border-radius:4px;background:none;padding:2px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:8px}
.grid button{text-align:left;padding:10px;min-height:46px}
.grid button.active{background:var(--accent2);border-color:var(--accent)}
.power{display:flex;gap:10px;align-items:center}
.power button{min-width:110px;font-weight:600}
.status{color:var(--muted);font-size:13px}
.note{background:#3a3220;border:2px solid #7a6a30;border-radius:4px;padding:10px;margin-bottom:14px}
.list label{display:flex;gap:10px;align-items:center;padding:6px 0;border-bottom:1px solid var(--edge)}
.hidden{display:none!important}
progress{width:100%}
</style>
</head>
<body>
<header>
  <h1>MINECRAFT</h1>
  <div class="status" id="status">verbinde …</div>
</header>
<main>
  <div class="note hidden" id="setupNote">Setup-Modus: Das Schild ist noch mit keinem WLAN verbunden. Trage unten dein Netz ein, danach startet es neu und verbindet sich.</div>
  <div class="panel"><canvas id="preview" height="140"></canvas></div>

  <div class="tabs">
    <button data-tab="control" class="active">Steuerung</button>
    <button data-tab="modes">Modi</button>
    <button data-tab="settings">Einstellungen</button>
  </div>

  <section id="tab-control">
    <div class="panel">
      <div class="power">
        <button id="power" class="primary">An</button>
        <div class="row" style="flex:1;margin:0">
          <label for="brightness">Helligkeit</label>
          <input type="range" id="brightness" min="1" max="255">
          <output id="brightnessOut"></output>
        </div>
      </div>
    </div>
    <div class="panel">
      <h2>Effekt</h2>
      <div class="grid" id="effects"></div>
    </div>
    <div class="panel">
      <h2>Parameter</h2>
      <div class="row"><label for="speed">Geschwindigkeit</label><input type="range" id="speed" min="0" max="255"><output id="speedOut"></output></div>
      <div class="row" id="intensityRow"><label for="intensity" id="intensityLabel">Intensität</label><input type="range" id="intensity" min="0" max="255"><output id="intensityOut"></output></div>
      <div class="row" id="color1Row"><label for="color1">Farbe 1</label><input type="color" id="color1"></div>
      <div class="row" id="color2Row"><label for="color2">Farbe 2</label><input type="color" id="color2"></div>
    </div>
  </section>

  <section id="tab-modes" class="hidden">
    <div class="panel">
      <h2>Taster-Reihenfolge</h2>
      <p class="status">Kurz drücken schaltet durch die markierten Effekte. Keine Markierung = alle.</p>
      <div class="row"><button id="cycleAll">Alle</button><button id="cycleNone">Keine</button></div>
      <div class="list" id="cycleList"></div>
    </div>
    <div class="panel">
      <h2>Automatischer Wechsel</h2>
      <div class="row"><label for="autoCycle">Wechseln alle</label>
        <select id="autoCycle">
          <option value="0">aus</option><option value="10">10 s</option><option value="30">30 s</option>
          <option value="60">1 min</option><option value="300">5 min</option><option value="900">15 min</option><option value="3600">1 h</option>
        </select></div>
    </div>
    <div class="panel">
      <h2>Bedienung am Gerät</h2>
      <p class="status">Aus: kurz drücken = an (letzter Zustand).<br>An: kurz drücken = nächster Effekt, doppelt drücken = Helligkeitsstufe, lang drücken = aus (Zustand wird gespeichert).</p>
    </div>
  </section>

  <section id="tab-settings" class="hidden">
    <div class="panel">
      <h2>WLAN</h2>
      <div class="row"><label for="ssid">Netz</label><input type="text" id="ssid" list="ssidList" autocomplete="off"><datalist id="ssidList"></datalist><button id="scan">Suchen</button></div>
      <div class="row"><label for="wpass">Passwort</label><input type="password" id="wpass" placeholder="unverändert"></div>
      <div class="row"><button class="primary" id="saveWifi">Speichern &amp; neu starten</button></div>
    </div>
    <div class="panel">
      <h2>Gerät</h2>
      <div class="row"><label for="hostname">Name</label><input type="text" id="hostname" pattern="[a-z0-9-]+"></div>
      <div class="row"><label for="powerOn">Nach Stromausfall</label>
        <select id="powerOn"><option value="last">letzter Zustand</option><option value="on">immer an</option><option value="off">immer aus</option></select></div>
      <div class="row"><button class="primary" id="saveDevice">Speichern</button></div>
    </div>
    <div class="panel">
      <h2>Strom</h2>
      <p class="status">Geschätzt gerade: <b id="currentMa">–</b> mA</p>
      <div class="row"><label for="mA">Limit (mA)</label><input type="number" id="mA" min="200" max="5000" step="100"></div>
      <div class="row"><label for="chMa">Je Farbkanal (mA)</label><input type="number" id="chMa" min="0.5" max="30" step="0.01"></div>
      <div class="row"><label for="baseMa">Grundlast (mA)</label><input type="number" id="baseMa" min="0" max="1000" step="1"></div>
      <div class="row"><button class="primary" id="savePower">Speichern</button></div>
      <h2 style="margin-top:16px">Kalibrieren mit dem Netzteil</h2>
      <p class="status">Das Testbild läuft ohne Begrenzung und endet nach 3 min von selbst.</p>
      <div class="row"><button id="calOff">1. Alle LEDs aus</button><input type="number" id="calOffMa" placeholder="gemessen mA" min="0"></div>
      <div class="row"><button id="calWhite">2. Alle weiß (50 %)</button><input type="number" id="calWhiteMa" placeholder="gemessen mA" min="0"></div>
      <div class="row"><button class="primary" id="calApply">Übernehmen</button><button id="calEnd">Testbild beenden</button></div>
    </div>
    <div class="panel">
      <h2>MQTT / Home Assistant</h2>
      <div class="row"><label for="mqOn">Aktiv</label><input type="checkbox" id="mqOn"></div>
      <div class="row"><label for="mqHost">Server</label><input type="text" id="mqHost"></div>
      <div class="row"><label for="mqPort">Port</label><input type="number" id="mqPort" min="1" max="65535"></div>
      <div class="row"><label for="mqUser">Benutzer</label><input type="text" id="mqUser" autocomplete="off"></div>
      <div class="row"><label for="mqPass">Passwort</label><input type="password" id="mqPass" placeholder="unverändert"></div>
      <div class="row"><label for="mqTopic">Topic</label><input type="text" id="mqTopic"></div>
      <div class="row"><label for="mqDisc">HA-Discovery</label><input type="checkbox" id="mqDisc"></div>
      <div class="row"><button class="primary" id="saveMqtt">Speichern</button></div>
    </div>
    <div class="panel">
      <h2>Firmware</h2>
      <div class="row"><input type="file" id="fw" accept=".bin"><button id="upload">Hochladen</button></div>
      <progress id="fwProgress" class="hidden" max="100" value="0"></progress>
      <div class="row"><button id="reboot">Neustart</button><button class="danger" id="factory">Werkseinstellungen</button></div>
    </div>
  </section>
</main>
<script>
const $ = id => document.getElementById(id);
let effects = [], layout = null, state = null, dragging = null;

async function api(path, body) {
  const opt = body === undefined ? {} : {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(body)};
  const r = await fetch('/api/' + path, opt);
  return r.headers.get('content-type')?.includes('json') ? r.json() : r.text();
}

// Kommandos sammeln, damit Schieberegler den ESP nicht fluten
let pending = {}, timer = null;
function send(patch) {
  Object.assign(pending, patch);
  Object.assign(state, patch);
  clearTimeout(timer);
  timer = setTimeout(() => { const p = pending; pending = {}; api('state', p); }, 80);
  render();
}

function render() {
  if (!state) return;
  $('power').textContent = state.on ? 'An' : 'Aus';
  $('power').classList.toggle('primary', state.on);
  for (const k of ['brightness', 'speed', 'intensity']) {
    if (dragging !== k) $(k).value = state[k];
    $(k + 'Out').textContent = state[k];
  }
  if (document.activeElement !== $('color1')) $('color1').value = state.color1;
  if (document.activeElement !== $('color2')) $('color2').value = state.color2;
  const fx = effects.find(e => e.id === state.effect) || {};
  $('intensityRow').classList.toggle('hidden', !fx.intensity);
  $('intensityLabel').textContent = fx.intensity || 'Intensität';
  $('color1Row').classList.toggle('hidden', !fx.color1);
  $('color2Row').classList.toggle('hidden', !fx.color2);
  document.querySelectorAll('#effects button').forEach(b => b.classList.toggle('active', b.dataset.id === state.effect));
  document.querySelectorAll('#cycleList input').forEach(c => c.checked = state.cycle.includes(c.value));
  $('autoCycle').value = state.autoCycle;
}

function buildEffects() {
  const grid = $('effects'), list = $('cycleList');
  for (const e of effects) {
    const b = document.createElement('button');
    b.textContent = e.name;
    b.dataset.id = e.id;
    b.onclick = () => send({effect: e.id, on: true});
    grid.appendChild(b);
    const l = document.createElement('label');
    l.innerHTML = `<input type="checkbox" value="${e.id}"> ${e.name}`;
    l.querySelector('input').onchange = saveCycle;
    list.appendChild(l);
  }
}

function saveCycle() {
  const ids = [...document.querySelectorAll('#cycleList input:checked')].map(c => c.value);
  send({cycle: ids});
}

// Vorschau: LEDs an ihren echten Positionen
async function preview() {
  const cv = $('preview');
  if (layout && !document.hidden) {
    try {
      const hex = await api('frame');
      const w = cv.clientWidth, h = Math.round(w * 0.22);
      if (cv.width !== w * 2) { cv.width = w * 2; cv.height = h * 2; cv.style.height = h + 'px'; }
      const ctx = cv.getContext('2d'), pad = 16;
      const sx = (cv.width - pad * 2) / layout.width, sy = (cv.height - pad * 2) / layout.height, s = Math.min(sx, sy);
      const ox = (cv.width - layout.width * s) / 2, oy = (cv.height - layout.height * s) / 2;
      ctx.fillStyle = '#0c0c0e'; ctx.fillRect(0, 0, cv.width, cv.height);
      const r = Math.max(2, s * 0.32);
      layout.leds.forEach(([x, y], i) => {
        const c = '#' + hex.substr(i * 6, 6);
        const px = ox + x * s, py = cv.height - (oy + y * s);
        ctx.fillStyle = '#1e1e24'; ctx.beginPath(); ctx.arc(px, py, r, 0, 7); ctx.fill();
        ctx.shadowColor = c; ctx.shadowBlur = r * 2.5; ctx.fillStyle = c;
        ctx.beginPath(); ctx.arc(px, py, r, 0, 7); ctx.fill(); ctx.shadowBlur = 0;
      });
    } catch (e) {}
  }
  setTimeout(preview, 150);
}

async function refresh() {
  try {
    if (Object.keys(pending).length === 0) { state = await api('state'); render(); }
    const info = await api('info');
    $('currentMa').textContent = info.currentMa + (info.calibration ? ' (Testbild ' + (info.calibration === 'white' ? 'weiß' : 'aus') + ')' : '');
    $('status').textContent = info.setupMode && !info.wifi
      ? `Setup-Netz „${info.apSsid}“ · ${info.ip}`
      : `${info.hostname}.local · ${info.ip} · WLAN ${info.wifi} (${info.rssi} dBm) · MQTT ${info.mqtt ? 'verbunden' : 'aus'} · v${info.version}`;
    $('setupNote').classList.toggle('hidden', !(info.setupMode && !info.wifi));
    if (info.setupMode && !info.wifi && !refresh.shown) { refresh.shown = true; showTab('settings'); }
  } catch (e) { $('status').textContent = 'keine Verbindung …'; }
  setTimeout(refresh, 2500);
}

async function loadSettings() {
  const s = await api('settings');
  $('ssid').value = s.wifi.ssid;
  $('wpass').placeholder = s.wifi.hasPass ? 'unverändert' : 'kein Passwort';
  $('hostname').value = s.hostname;
  $('mA').value = s.maxMilliamps;
  $('chMa').value = (s.channelMicroamps / 1000).toFixed(2);
  $('baseMa').value = s.baseMilliamps;
  $('powerOn').value = s.powerOn;
  $('mqOn').checked = s.mqtt.enabled;
  $('mqHost').value = s.mqtt.host;
  $('mqPort').value = s.mqtt.port;
  $('mqUser').value = s.mqtt.user;
  $('mqPass').placeholder = s.mqtt.hasPass ? 'unverändert' : 'kein Passwort';
  $('mqTopic').value = s.mqtt.topic;
  $('mqDisc').checked = s.mqtt.discovery;
}

function showTab(name) {
  document.querySelectorAll('.tabs button').forEach(b => b.classList.toggle('active', b.dataset.tab === name));
  for (const t of ['control', 'modes', 'settings']) $('tab-' + t).classList.toggle('hidden', t !== name);
}

document.querySelectorAll('.tabs button').forEach(b => b.onclick = () => showTab(b.dataset.tab));
$('power').onclick = () => send({on: !state.on});
for (const k of ['brightness', 'speed', 'intensity']) {
  $(k).oninput = () => { dragging = k; send({[k]: +$(k).value}); };
  $(k).onchange = () => { dragging = null; };
}
$('color1').oninput = () => send({color1: $('color1').value});
$('color2').oninput = () => send({color2: $('color2').value});
$('autoCycle').onchange = () => send({autoCycle: +$('autoCycle').value});
$('cycleAll').onclick = () => { document.querySelectorAll('#cycleList input').forEach(c => c.checked = true); saveCycle(); };
$('cycleNone').onclick = () => { document.querySelectorAll('#cycleList input').forEach(c => c.checked = false); saveCycle(); };

$('scan').onclick = async () => {
  $('scan').textContent = 'Suche …';
  for (let i = 0; i < 20; i++) {
    const r = await api('scan');
    if (r.networks) {
      const dl = $('ssidList'); dl.innerHTML = '';
      [...new Map(r.networks.sort((a, b) => b.rssi - a.rssi).map(n => [n.ssid, n])).values()].forEach(n => {
        const o = document.createElement('option'); o.value = n.ssid; o.label = `${n.rssi} dBm${n.open ? ' · offen' : ''}`; dl.appendChild(o);
      });
      break;
    }
    await new Promise(r => setTimeout(r, 700));
  }
  $('scan').textContent = 'Suchen';
  $('ssid').focus();
};
$('saveWifi').onclick = async () => {
  const wifi = {ssid: $('ssid').value.trim()};
  if ($('wpass').value) wifi.pass = $('wpass').value;
  await api('settings', {wifi});
  alert('Gespeichert. Das Schild startet neu und verbindet sich mit „' + wifi.ssid + '“.\nDanach erreichbar unter http://' + $('hostname').value + '.local');
};
$('saveDevice').onclick = async () => {
  await api('settings', {hostname: $('hostname').value.trim().toLowerCase(), powerOn: $('powerOn').value});
  setTimeout(loadSettings, 500);
};
$('savePower').onclick = async () => {
  await api('settings', {maxMilliamps: +$('mA').value, channelMicroamps: Math.round(+$('chMa').value * 1000), baseMilliamps: +$('baseMa').value});
  setTimeout(loadSettings, 500);
};
$('calOff').onclick = () => api('calibrate', {mode: 'off'});
$('calWhite').onclick = () => api('calibrate', {mode: 'white'});
$('calEnd').onclick = () => api('calibrate', {mode: 'end'});
$('calApply').onclick = async () => {
  const off = +$('calOffMa').value, white = +$('calWhiteMa').value;
  if (!(white > off)) { alert('Bitte beide Messwerte eintragen (weiß muss größer als aus sein).'); return; }
  await api('settings', {calibration: {offMa: off, whiteMa: white}});
  await api('calibrate', {mode: 'end'});
  setTimeout(async () => {
    const before = $('chMa').value;
    await loadSettings();
    alert(`Übernommen: ${$('chMa').value} mA je Kanal, Grundlast ${$('baseMa').value} mA` + (before === $('chMa').value ? '\n(unverändert – Werte unplausibel?)' : ''));
  }, 600);
};
$('saveMqtt').onclick = async () => {
  const mqtt = {enabled: $('mqOn').checked, host: $('mqHost').value.trim(), port: +$('mqPort').value,
    user: $('mqUser').value.trim(), topic: $('mqTopic').value.trim(), discovery: $('mqDisc').checked};
  if ($('mqPass').value) mqtt.pass = $('mqPass').value;
  await api('settings', {mqtt});
  $('mqPass').value = '';
  setTimeout(loadSettings, 500);
};
$('upload').onclick = () => {
  const f = $('fw').files[0];
  if (!f) return;
  const fd = new FormData(); fd.append('firmware', f, f.name);
  const x = new XMLHttpRequest(), p = $('fwProgress');
  p.classList.remove('hidden');
  x.upload.onprogress = e => p.value = e.loaded / e.total * 100;
  x.onload = () => alert(x.status === 200 ? 'Update erfolgreich, Neustart …' : 'Update fehlgeschlagen');
  x.open('POST', '/api/update'); x.send(fd);
};
$('reboot').onclick = () => api('reboot', {});
$('factory').onclick = () => { if (confirm('Alle Einstellungen inkl. WLAN löschen?')) api('factory-reset', {}); };

(async () => {
  [effects, layout] = await Promise.all([api('effects'), api('layout')]);
  buildEffects();
  refresh();
  loadSettings();
  preview();
})();
</script>
</body>
</html>
)HTML";
