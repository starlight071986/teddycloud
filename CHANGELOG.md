# Changelog

Versionsschema: `<Original-Version>-fork.<n>`. `n` zählt die Fork-Releases je Original-Version.
Git-Tag `tc_v<Version>`, Docker-Tag `starlight1986/teddycloud:<Version>` (neuestes zusätzlich `latest`).

## [0.7.0-fork.1] - 2026-09-28

Basis: Original [tc_v0.7.0](https://github.com/toniebox-reverse-engineering/teddycloud/releases/tag/tc_v0.7.0)

### Hinzugefügt
- REST-Endpunkt `GET /api/box/cmd?boxId=&cmd=&value=` für `stop`, `volLimitSpk`, `volLimitHdp`,
  `led`, `slapEnabled`, `slapDir`. War in MQTT_CONTROL.md dokumentiert, fehlte aber im Original.
- Beispiel-Deployment mit Caddy-Reverse-Proxy und Login (`deploy/`).

### Geändert
- `tonie_json.cache_images` ist ab Werk an: Figurenbilder werden beim ersten Aufruf lokal
  gespeichert und danach ohne Online-CDN ausgeliefert.

### Sicherheit
- Hinweis: Der Web-Port des Originals hat keine Zugriffskontrolle. Nur hinter einem
  Reverse-Proxy mit Login betreiben (siehe README).
