# 0002: Bedienung am Taster und Speicherung

- Status: angenommen
- Datum: 2026-11-01

## Kontext

Das Schild hat einen Taster. Nach dem Ausschalten soll der letzte Zustand erhalten bleiben. WLAN-Zugangsdaten dürfen nicht im Code stehen.

## Entscheidung

Taster (gegen GND, interner Pull-up, 30 ms Entprellung):

| Zustand | Aktion | Wirkung |
|---|---|---|
| aus | kurz oder doppelt | an, mit gespeichertem Effekt und Einstellungen |
| aus | lang | nichts |
| an | kurz | nächster Effekt aus der Taster-Liste (leer = alle) |
| an | doppelt | nächste Helligkeitsstufe (32, 80, 160, 255, dann wieder 32) |
| an | lang (≥ 0,8 s, löst schon beim Halten aus) | aus, Zustand sofort speichern |

Ein Einzelklick wird erst 300 ms nach dem Loslassen gemeldet, weil er sonst nicht von einem Doppelklick zu unterscheiden wäre.

Speicherung (NVS-Flash über Preferences):

- Ausschalten per Taster speichert sofort.
- Alle anderen Änderungen (Web, MQTT, Taster) werden 5 s nach der letzten Änderung gespeichert, um den Flash zu schonen.
- Der automatische Effektwechsel speichert nicht.
- Verhalten nach dem Einstecken einstellbar: letzter Zustand (Standard), immer an, immer aus.

WLAN:

- Ohne gespeicherte Zugangsdaten startet ein offener Access Point `Minecraft-Sign-XXXX` mit Captive Portal.
- Klappt die Verbindung beim Start 20 s lang nicht oder bricht sie im Betrieb für 60 s ab, kommt der Access Point dazu. Alle 30 s wird erneut verbunden, solange niemand im Setup-Netz ist.
- Ändert sich die SSID ohne neues Passwort, wird das alte Passwort gelöscht. Gleiches gilt für den MQTT-Server.

## Verworfene Alternativen

- Jede Änderung sofort speichern: Beim Ziehen eines Schiebereglers entstünden Dutzende Schreibvorgänge.
- Access Point mit festem Passwort: Ein Passwort müsste im Code stehen oder auf das Gerät geklebt werden. Das Setup-Netz ist nur aktiv, wenn kein WLAN erreichbar ist.

## Folgen

- Wird der Strom innerhalb von 5 s nach einer Änderung über Web oder MQTT getrennt, geht diese Änderung verloren.
- Das Setup-Netz ist offen; wer in Reichweite ist, während es aktiv ist, kann das WLAN umstellen.
