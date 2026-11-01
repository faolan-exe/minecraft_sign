#pragma once

// MQTT mit Home-Assistant-Discovery. Läuft in einem eigenen Task, damit ein
// hängender Verbindungsaufbau die Animationen nicht anhält.
//
// Topics (<t> = eingestelltes Basis-Topic):
//   <t>/status   online/offline (retained, Last Will)
//   <t>/state    Zustand als JSON (retained)
//   <t>/set      Kommando als JSON, gleiches Format wie POST /api/state
namespace mqtt {

void begin();
bool connected();

}  // namespace mqtt
