# TeddyCloud Fork

Eigene Anpassungen an [TeddyCloud](https://github.com/toniebox-reverse-engineering/teddycloud),
gepflegt als **Patch-Reihe**, die sich auf jede neue Original-Version anwenden lässt.

- GitHub: https://github.com/starlight071986/teddycloud
  - Branch `tooling` (Standard): dieses README, [CHANGELOG](CHANGELOG.md), Patches, Skripte
  - Branch `fork/tc_vX.Y.Z`: Original-Version + Patches, direkt baubar
  - Branch `master`: unverändertes Original
- Docker Hub: [`starlight1986/teddycloud`](https://hub.docker.com/r/starlight1986/teddycloud)
  - `latest` = neuestes Fork-Release, `X.Y.Z-fork.N` = feste Version (amd64 + arm64)

## Aufbau

```
teddycloud/
├── patches/
│   ├── server/        Patches für das TeddyCloud-Repo (Quelle der Wahrheit)
│   └── web/           Patches für das Frontend (Submodul teddycloud_web), derzeit leer
├── scripts/
│   ├── new-version.sh     Original-Version klonen + Patches anwenden
│   ├── export-patches.sh  Commits des Branches "fork" zurück nach patches/ schreiben
│   ├── build.sh           Docker-Image bauen (lokal oder Multi-Arch + Push)
│   └── release.sh         Release taggen und nach GitHub pushen
├── deploy/            Beispiel: TeddyCloud hinter Caddy mit Login
└── tc_vX.Y.Z/         Ein Ordner pro Original-Version (eigenes Git-Repo)
                       Tag tc_vX.Y.Z = Original, Branch "fork" = Original + Patches
```

Das Original bleibt in jedem Versionsordner unverändert erreichbar:
`git -C tc_v0.7.0 diff tc_v0.7.0 fork` zeigt genau unsere Änderungen.

## Patches

| Patch | Inhalt |
|---|---|
| `0001-api-add-api-box-cmd-REST-endpoint` | `GET /api/box/cmd?boxId=&cmd=&value=` (in MQTT_CONTROL.md dokumentiert, im Original aber nicht vorhanden). Eigene Datei `src/handler_api_fork.c`, nur 2 Zeilen in `server.c`. |
| `0002-settings-cache-figurine-images-locally-by-default` | `tonie_json.cache_images` ab Werk an: Bilder werden beim ersten Aufruf lokal gespeichert. Bestehende Installationen behalten ihren gespeicherten Wert. |

Befehle für `/api/box/cmd`: `stop`, `volLimitSpk` (0–3), `volLimitHdp` (0–3), `led` (0–2),
`slapEnabled` (true/false), `slapDir` (true/false). `boxId` = Common Name der Box.
Wirkung erst beim nächsten Freshness-Check der Box.

## Neue Original-Version übernehmen

```bash
git ls-remote --tags https://github.com/toniebox-reverse-engineering/teddycloud | grep tc_v
scripts/new-version.sh tc_v0.7.1
scripts/build.sh tc_v0.7.1
```

Bei einem Konflikt bricht das Skript ab und nennt die Schritte (`git am --continue`,
danach `scripts/export-patches.sh <version>`).

## Änderungen entwickeln

1. Im Versionsordner auf Branch `fork` arbeiten und committen (ein Commit = ein Patch).
2. `scripts/export-patches.sh tc_v0.7.0` schreibt `patches/` neu.
3. Frontend-Änderungen im Submodul `teddycloud_web` auf dessen Branch `fork` committen;
   vor dem Image-Build `make web` ausführen (braucht npm), da das Image das fertig
   gebaute Frontend aus `contrib/data/www/web` nimmt.

Regeln, damit Patches bei Updates selten kollidieren:
- Neuen Code in eigene Dateien (`*_fork.c`, `*_fork.h`); in Original-Dateien nur Einhängepunkte.
- Keine Formatierungs- oder Aufräumänderungen an Original-Code.

## Release

1. `CHANGELOG.md` um einen Abschnitt `## [0.7.0-fork.N] - JJJJ-MM-TT` ergänzen.
2. Tooling committen (`git add -A && git commit`).
3. Taggen und nach GitHub pushen, dann Image bauen und nach Docker Hub pushen:

```bash
scripts/release.sh tc_v0.7.0 2
scripts/build.sh tc_v0.7.0 --push
```

`scripts/build.sh tc_v0.7.0` ohne `--push` baut nur lokal (`teddycloud-fork:dev`).

Commit-Identität: Jedes Repo hat eine lokale `user.email` (GitHub-noreply-Adresse);
`new-version.sh` übernimmt sie aus diesem Repo. Nie mit der Firmenadresse committen.

## Sicherheit

TeddyCloud hat auf dem Web-Port (80/8443) **keine Zugriffskontrolle**: Der Login ist im
Original nur ein Platzhalter, alle `/api/*`-Aufrufe sind offen. Deshalb:

- Web-Port nie direkt ins Internet, auch im LAN nur über einen Reverse-Proxy mit Login
  (Beispiel in `deploy/`: Caddy mit Basic-Auth).
- Port 443 (Box-API) geht direkt an TeddyCloud; die Boxen melden sich per Client-Zertifikat.
  `/api/*` ist auf diesem Port nicht erreichbar.
- Home Assistant bevorzugt über MQTT anbinden (Broker mit Benutzer/Passwort).
  Für REST in Home Assistant: `rest_command` mit `username`/`password` des Proxys.
