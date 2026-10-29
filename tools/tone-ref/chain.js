/* The sound chain of reference/skurkymello.html, for Tone.Offline. buildChain() is buildSynth() (lines 1246-1368)
 * with the React refs replaced by a plain parameter object; the helpers are lines 1125-1146 and the triggers
 * follow triggerNote() (1582-1669, SINGLE gate, no ratchets) and triggerSub() (1671-1678). Keep this a copy:
 * when the HTML changes, change this to match, line by line. Runs in the page, after Tone.js. */
"use strict";

const DEFAULTS = { // useState defaults, lines 485-538
  slideTime: 0.05, filterCut: 0.7, filterRes: 0.2, attack: 0.005, decay: 0.18, sustain: 0.4, release: 0.12,
  oscType: "sawtooth", pwmWidth: 0.5, subOscOn: true, subOscLevel: 0.22, subOscOct: -1, subOscWave: "sine",
  reverbWet: 0.24, reverbSize: 0.7, reverbDecay: 6.0, reverbPre: 0.012, reverbTone: 0.62, reverbLevel: 1.15,
  delayWet: 0.12, delaySync: true, delayDiv: "8n", delayTime: 0.25, delayFb: 0.32, delayTone: 0.72, delayLevel: 0.75,
  distAmt: 0, distMix: 1, distCharacter: 0.35, distTone: 0.82, distLevel: 0.9, masterVol: 0.75,
  subOn: true, subLevel: 0.42, subWave: "sine", subDecay: 0.16, subRelease: 0.12, subGlide: 0.02, subCut: 0.45,
  subDrive: 0,
};

const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
function logFilterHz(v) { return 80 * Math.pow(200, v); }
function reverbSendGainFromWet(wet) {
  const w = clamp(wet, 0, 1);
  if (w <= 0) return 0;
  return clamp(0.03 + Math.pow(w, 1.2) * 0.95, 0, 1.05);
}
function reverbReturnGain(level) { return clamp(level, 0, 3) * 1.05; }
function distDriveFromAmount(amt, char) {
  return clamp(Math.pow(clamp(amt, 0, 1), 1.9) * (0.28 + 0.52 * clamp(char, 0, 1)), 0, 0.8);
}
function reverbToneHz(v) { return 700 * Math.pow(18, clamp(v, 0, 1)); }
function delayToneHz(v) { return 450 * Math.pow(26, clamp(v, 0, 1)); }
function distToneHz(v) { return 320 * Math.pow(38, clamp(v, 0, 1)); }
function subFilterHz(v) { return 40 * Math.pow(55, clamp(v, 0, 1)); }

/* fx: "full" (the HTML), "dry" (delay and reverb sends 0), "reverb" (delay send 0). */
async function buildChain(P, fx) {
  const synthOsc = P.oscType === "pulse" ? { type: "pulse", width: clamp(P.pwmWidth, 0.05, 0.95) }
                                         : { type: P.oscType === "off" ? "sine" : P.oscType };
  const env = { attack: P.attack, decay: P.decay, sustain: P.sustain, release: P.release };
  const synth = new Tone.Synth({ oscillator: synthOsc, envelope: env });
  const subOsc = new Tone.Synth({ oscillator: { type: P.subOscWave }, envelope: env });
  const sub = new Tone.Synth({
    oscillator: { type: P.subWave },
    envelope: { attack: 0.003, decay: P.subDecay, sustain: 0.24, release: P.subRelease },
  });
  const subOscGain = new Tone.Gain(P.subOscOn ? clamp(P.subOscLevel, 0, 1) : 0);
  sub.set({ portamento: P.subGlide });
  const subFilter = new Tone.Filter({ frequency: subFilterHz(P.subCut), type: "lowpass", rolloff: -24, Q: 0.8 });
  const subDriveNode = new Tone.Distortion({ distortion: P.subDrive, wet: P.subDrive > 0 ? 1 : 0, oversample: "2x" });
  const subGain = new Tone.Gain(P.subLevel);
  const mix = new Tone.Gain(0.58);
  synth.set({ portamento: P.slideTime });
  const filter = new Tone.Filter({ frequency: logFilterHz(P.filterCut), type: "lowpass", rolloff: -24, Q: P.filterRes * 15 });
  const dist = new Tone.Distortion({
    distortion: distDriveFromAmount(P.distAmt, P.distCharacter), wet: clamp(P.distMix, 0, 1), oversample: "4x",
  });
  const distTone = new Tone.Filter({ type: "lowpass", frequency: distToneHz(P.distTone), Q: 0.2, rolloff: -24 });
  const distLevelNode = new Tone.Gain(clamp(P.distLevel, 0.2, 1.4));
  const dry = new Tone.Gain(0.84);
  const delay = new Tone.FeedbackDelay({ delayTime: P.delaySync ? P.delayDiv : P.delayTime, feedback: P.delayFb, wet: 1 });
  const dlySend = new Tone.Gain(fx === "full" ? P.delayWet : 0);
  const dlyTone = new Tone.Filter({ type: "lowpass", frequency: delayToneHz(P.delayTone), Q: 0.2, rolloff: -24 });
  const dlyReturn = new Tone.Gain(clamp(P.delayLevel, 0, 1.5));
  const reverb = new Tone.Reverb({ decay: clamp(P.reverbDecay, 0.4, 60), preDelay: P.reverbPre, wet: 1 });
  await reverb.ready; /* generate() renders the noise IR; the HTML calls it and lets it finish in the background */
  const revSend = new Tone.Gain(fx === "dry" ? 0 : reverbSendGainFromWet(P.reverbWet));
  const revTone = new Tone.Filter({ type: "lowpass", frequency: reverbToneHz(P.reverbTone), Q: 0.2, rolloff: -24 });
  const revHPF = new Tone.Filter({ type: "highpass", frequency: 24, Q: 0.35, rolloff: -24 });
  const revReturn = new Tone.Gain(reverbReturnGain(P.reverbLevel));
  const fxMix = new Tone.Gain(1);
  const limiter = new Tone.Limiter(-1);
  synth.connect(mix);
  subOsc.connect(subOscGain);
  subOscGain.connect(mix);
  sub.connect(subFilter);
  subFilter.connect(subDriveNode);
  subDriveNode.connect(subGain);
  subGain.connect(mix);
  mix.connect(filter);
  filter.connect(dist);
  dist.connect(distTone);
  distTone.connect(distLevelNode);
  distLevelNode.connect(dry);
  dry.connect(fxMix);
  distLevelNode.connect(dlySend);
  dlySend.connect(delay);
  delay.connect(dlyTone);
  dlyTone.connect(dlyReturn);
  dlyReturn.connect(fxMix);
  distLevelNode.connect(revSend);
  revSend.connect(reverb);
  reverb.connect(revTone);
  revTone.connect(revHPF);
  revHPF.connect(revReturn);
  revReturn.connect(fxMix);
  fxMix.connect(limiter);
  limiter.toDestination();
  Tone.getDestination().volume.value = Tone.gainToDb(P.masterVol);
  return { synth, subOsc, sub };
}

/* triggerNote(), SINGLE gate with one ratchet: the lead and the follower sub get the same duration. */
function triggerLead(c, P, n) {
  const note = Tone.Frequency(n.midi, "midi").toNote();
  const subNote = Tone.Frequency(clamp(n.midi + P.subOscOct * 12, 12, 108), "midi").toNote();
  c.synth.portamento = 0; /* no slide steps in these stimuli */
  c.synth.triggerAttackRelease(note, n.dur, n.t, n.vel);
  if (P.subOscOn) c.subOsc.triggerAttackRelease(subNote, n.dur, n.t, n.vel * clamp(P.subOscLevel, 0, 1));
}
/* triggerSub(): fixed velocity 0.9, the dedicated sub's own glide. */
function triggerSub(c, P, n) {
  c.sub.portamento = P.subGlide;
  c.sub.triggerAttackRelease(Tone.Frequency(n.midi, "midi").toNote(), n.dur, n.t, 0.9);
}

/* A seeded Math.random, so the reverb's noise IR is the same on every run. render.mjs calls it before Tone.js loads:
 * Tone keeps its noise buffers once made, so each stimulus gets a fresh page. */
function seedRandom(seed) {
  let s = seed >>> 0;
  Math.random = () => {
    s = (s + 0x6d2b79f5) >>> 0;
    let t = s;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

/* Renders one stimulus; returns [left, right] as base64 Float32 for the page boundary. */
async function renderStimulus(st, sampleRate, bpm) {
  const P = Object.assign({}, DEFAULTS, st.params || {});
  const buf = await Tone.Offline(async () => {
    Tone.getTransport().bpm.value = bpm; /* the HTML's transport runs at the pattern BPM: "8n" delay follows it */
    const c = await buildChain(P, st.fx);
    for (const n of st.lead || []) triggerLead(c, P, n);
    for (const n of st.sub || []) if (P.subOn) triggerSub(c, P, n);
  }, st.seconds, 2, sampleRate);
  const enc = (f32) => {
    const b = new Uint8Array(f32.buffer, f32.byteOffset, f32.byteLength);
    let s = "";
    for (let i = 0; i < b.length; i += 0x8000) s += String.fromCharCode.apply(null, b.subarray(i, i + 0x8000));
    return btoa(s);
  };
  return [enc(buf.getChannelData(0)), enc(buf.getChannelData(1))];
}
