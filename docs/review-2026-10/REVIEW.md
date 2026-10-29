# SkurkyMello FM-1 – full gjennomgang og beslutning

Gjennomgått: `joakimskurk/fm1mello`, gren `visual-port-home`, commit `3cd776c` (Reverb V3 Candidate A.1),
mot originalen `skurkymello.html` (identisk med jhf.no-versjonen). Fire parallelle gjennomganger:
originalen, maskinvare-UX, firmware-kode og lydlikhet. Detaljrapportene ligger ved siden av denne fila:

- [01_original.md](01_original.md): hvorfor HTML-en føles bra, alle standardverdier, lydkjede, topp 10 som må kopieres
- [02_ux.md](02_ux.md): fullt kontrollkart for FM-1, friksjonsliste, nytt PERFORM HOME-oppsett
- [03_code.md](03_code.md): 1 P0, 4 P1, 12 P2 med host-tester
- [04_sound.md](04_sound.md): Tone.js-kjeden mot C-motoren, målt avvik og rekkefølge for DSP-endringer

## Beslutning: modifiser, ikke kast. Men tre deler skrives om.

Porten er ikke 60 % ferdig jevnt over. Den er nesten ferdig i noen lag og feil i andre:

| Lag | Status | Beslutning |
|---|---|---|
| Datamodell og standardverdier (skalatrinn, 80 BPM, C-moll, `i%7`, saw, envelope) | Riktig, matcher HTML | **Behold** |
| Sekvensering (pulser, ratchets, gate, sannsynlighet, seksjoner, TM, DOT, Loop Cycle, randomizer-tabell) | Riktig matematikk | **Behold**, legg til lookahead |
| Lagring (A/B-banker, CRC, commit-footer) | God grunnmur, men dirty-marking og rollback er feil | **Behold og fiks** |
| Lyd-DSP (`mello_audio.c`: filter, envelopes, gain-staging, FX, reverb) | Feil mål flere steder | **Skriv om** mot en referanse |
| UI-toppnivå (16 sider, 47 lag, HOME redigerer ett skjult steg) | Konsistent, men ikke umiddelbart | **Skriv om HOME**, slå sammen sider |
| Repo og arbeidsflyt (kildekode som base64/xz-lag, 15 workflows) | Hindrer alt annet | **Bygg på nytt** rundt eksisterende kode |

En full redo ville kastet den delen som faktisk virker (modell, sekvensering, timing-regler, lagring)
og gitt de samme problemene igjen. Avstanden til HTML-en sitter i **lyden** og **betjeningen**, ikke i
funksjonslisten.

## Hvorfor den ikke føles som HTML-en

1. **Lyden er ikke samme instrument.**
   - Filteret: HTML-en bruker to lowpass-biquader i serie med Q = res·15, og Web Audio tolker det som dB.
     Det gir et tydelig resonanstopp (+7 dB ved standard, +30 dB på maks). C-laddere har
     tilbakekobling på maks ~0,6 og har **ingen resonanstopp** på noen innstilling.
   - Envelopes: Tone bruker eksponentielle kurver, C bruker lineære. 50 ms etter note-off er HTML på 0,10 og C på 0,58.
     Det er derfor porten låter "lang og myk" der HTML-en er "plukkete".
   - Gain-staging: HTML-ens distortion er alltid på og ganger med 1/3 ved AMT 0. C hopper over den og ligger
     rundt 9,5 dB varmere, rett inn i den harde klippen på −1 dBFS.
   - Reverb: HTML-en bruker Tone.Reverb, altså konvolusjon med hvit støy (RT60 ≈ 2,5 s ved DECAY 6 s, flatt spektrum).
     SIZE gjør ingenting der. WP-E3 har jaktet på en "bedre" FDN mot et mål som aldri var referansen.
   - I tillegg: aliasende oscillatorer, FX-tonefiltre på 6 dB/okt i stedet for 24, pitch-LFO 4–5× for grunn,
     follower-sub 13 dB for sterk, ingen parameterutjevning.
2. **Keyboardet lager aldri lyd.** Taster velger posisjoner eller utløser handlinger. Det finnes ingen
   audition, transponering eller innspilling.
3. **HOME er ikke et instrument.** K1–K4 redigerer ett skjult valgt steg. Cutoff, decay, reverb og drive
   ligger 2–4 trykk unna. RANDOM krever to umerkede svarte taster.
4. **Skjermen lyver.** "REC REROLL", "EDIT+REC CLEAR" og "REC TAP" vises, men REC gjør ingenting
   (`mello_ui.c:1292`, `:2106`, `:2239`). SEL+note på STEP-siden går ikke fordi SEL bytter side først.
5. **Timing.** Det er ingen lookahead, så hvert steg starter 0–5,8 ms for sent, og første ratchet-gap blir kortere enn resten.

## Feil som må fikses før første flash

| # | Alvor | Feil | Bevis |
|---|---|---|---|
| 1 | **P0** | `q15mul`-overflyt i ladder-filteret. SQUARE + follower SQUARE nivå 1000, cutoff ≥850 → utgangen låses på −21903 (≈ −3,5 dBFS DC) til omstart | Reprodusert av meg og av kodeagenten |
| 2 | **P1** | Reverb-DC (−150) holder `tail_guard` i live, så `silent` blir aldri sann. Da blokkeres autosave, SAVE og sample-opplasting med fabrikkoppsettet | Kodeagent; DC reprodusert av meg |
| 3 | **P1** | Endringer i steg, lyd og FX markerer ikke lagring som skitten, så de forsvinner ved strøm av | Verifisert i kode (`persistent_dirty` kalles bare 8 steder) |
| 4 | **P1** | Tilbakerulling av firmware sletter prosjektet i nyere format (to autosaves visker begge banker) | Kodeagent, RAM-flashmodell |
| 5 | **P1** | Chord Machine-innstillinger lagres aldri | Kodeagent |
| 6 | **P1** | Sample-dekoding i lyd-callbacken har ingen øvre grense (målt 2,3× normal blokktid) | Kodeagent |
| 7 | P2 | MIDI CC113 på kanal 16 kan slette en sample-slot | Kodeagent |
| 8 | P2 | Hver lagring sletter 10 flash-sektorer for ~0,8 KB og fryser hovedløkken i 0,5–4 s | Kodeagent |

Fiks for 1 og 2: Ladder-tilstandene må klemmes, eller regnes i 64-bit. AMP-LFO skal bare dempe. DC-blokkeren skal
skiftes ut med den dødsonefrie varianten i `reverb-dc-blocker-fix.patch`. Dette er uansett mest et bevis på
at lyd-DSP-en må skrives om (Fase 2).

## Plan

### Fase 0 – Rydd grunnen (1–2 dager)
- Stopp WP-E3. Reverb V3 jakter på feil mål.
- Hent ut den endelige kildekoden som vanlige filer i et rent repo, eller ny `main`, og kjør clang-format.
  Én workflow via `zvenson/fm1-build`, pluss lokalt bygg på Mac med Docker.
- Jobb lokalt med Claude Code: én endring, ett bygg, én test. Ingen base64-patcher.

### Fase 1 – Trygt på maskinvare (2–3 dager)
- Fiks P0 og P1-ene i tabellen over.
- Første flash av et rent bygg på en ekte FM-1: installering fra V15, OCT−-redning, tilbake til V15,
  lagring og omstart, 10 min full last.

### Fase 2 – Lyd = HTML (1–2 uker)
- **Referanse først:** render HTML-en med `Tone.Offline` i headless Chromium, som CI allerede har, for faste
  stimuli. Lag golden-filer, og sammenlign C-renderingen mot dem (spektrum, envelope, nivå). Da blir
  "feel" målbart.
- Rekkefølge, med kostnad målt av lydagenten (~+60–100 sykler/sample av ~330 ledig):
  1. Ladder → to biquader med Q i dB, og klipp på ±1
  2. Tone-envelopes (eksponentielle) for lead, sample og chord. Sub-koden har dem allerede.
  3. Alltid-på waveshaper med Tones kurve og gain-staging (1/3)
  4. Transport-lookahead på 12–20 ms
  5. Reverb mot støykonvolusjons-målet: RT60 = 6,9·ln(decay+1)/ln 200, flatt spektrum, tett start
- Deretter (+100–180 sykler): PolyBLEP-oscillatorer, 24 dB FX-tonefiltre, LFO-dybde, follower-nivå,
  eksponentiell portamento, myk limiter, parameterutjevning.

### Fase 3 – Feel = HTML (1–2 uker)
- **PERFORM HOME** (detaljer i 02_ux.md §5):
  - Hvite taster: trykk slår et steg av/på, hold og vri redigerer det. Tast-LED-ene viser playhead.
  - K1–K4 er faste: CUTOFF, DECAY, SPACE, DRIVE.
  - SELECT, ALGORITHM og PRESETS er TEMPO, INTENSITY og PATTERN (køet til neste takt).
  - Svarte taster: RANDOM, UNDO, MUTATE, FILL, SUB-MUTE, REPEAT, REVERSE, RESTART, transponering ±, og CLEAR (hold).
  - EDIT gjør tastene til et kromatisk keyboard med lyd. REC gjør steg-innspilling.
- Høyst to nivåer. Slå sammen LFO/DOT/TURING, SUB/CHORD og sekvenssidene til sider med lag.
- Fiks hjelpetekstene, SEL-feilen, verdier i Hz og notenavn, og akselerasjon på encodere.
- Autosave på alle endringer.

### Fase 4 – Validering og videre
- Gå gjennom hele `HARDWARE_VALIDATION_CHECKLIST.md` på enhet.
- Først deretter: Undo Last Random (WP-F), ANALOG/FM og annet nytt.

## Hva HTML-en gjør som porten må beholde (topp 10, fra 01_original.md)
1. Tonehøyde som skalatrinn, og de 27 tastene mappet til trinn. Feil tone skal være umulig.
2. RANDOM med nøyaktig samme sannsynlighetstabell og ALL/NONE-maske
3. Musikalske standardverdier og én knapp til lyd
4. FX-topologi og gain-staging (parallelle send-returer, limiter, −2,5 dB master)
5. Leadstemme: båndbegrenset saw, to filtertrinn med Q i dB
6. Puls-, ratchet- og gate-regler, med noter planlagt i forkant
7. Sub-bass som egen stemme
8. Vedvarende 16-kolonners notevisning med playhead
9. Generative lag i toneart, med subtile standardverdier
10. Myke ramper, slot-bytte køet til slutten av loopen, trygg live-justering

## Usikkerhet
- HTML-ens utgangsnivå (≈ −21 dBFS) og Limiter-kneet er beregnet ut fra Tone 14-kildekoden, ikke målt.
  Referanserenderen i Fase 2 avgjør det.
- LFO→AMP-varianten av P0 er rapportert av kodeagenten. Jeg fikk den ikke til med én tone.
- Lagringspåstandene (#3, #4) er fra kodelesing og RAM-modell, ikke fra enhet.
- Ingenting er testet på en ekte FM-1.
