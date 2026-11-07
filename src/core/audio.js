/**
 * audio — procedural WebAudio engine, no sample assets.
 *
 * A hover racer lives or dies on its engine note, so the drone is two detuned
 * saws + a sub through a speed-modulated lowpass, with one-shot synths for
 * gates, boost, impacts and the Tachyon Surge.
 */

export class AudioEngine {
  constructor() {
    this.ctx = null;
    this.ready = false;
    this.muted = false;
    this.master = null;
  }

  /** Must be called from a user gesture. */
  init() {
    if (this.ctx) return;
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    this.ctx = new AC();
    const ctx = this.ctx;

    this.master = ctx.createGain();
    this.master.gain.value = 0.0001;
    this.master.connect(ctx.destination);

    // --- engine drone -------------------------------------------------------
    this.engineGain = ctx.createGain();
    this.engineGain.gain.value = 0.0;
    this.engineFilter = ctx.createBiquadFilter();
    this.engineFilter.type = 'lowpass';
    this.engineFilter.frequency.value = 520;
    this.engineFilter.Q.value = 6;
    this.engineGain.connect(this.engineFilter);
    this.engineFilter.connect(this.master);

    this.oscs = [];
    const ratios = [1, 1.005, 2.01];
    const gains = [0.5, 0.42, 0.22];
    for (let i = 0; i < ratios.length; i++) {
      const o = ctx.createOscillator();
      o.type = i === 2 ? 'square' : 'sawtooth';
      o.frequency.value = 42;
      const g = ctx.createGain();
      g.gain.value = gains[i];
      o.connect(g);
      g.connect(this.engineGain);
      o.start();
      this.oscs.push({ o, ratio: ratios[i] });
    }

    // sub
    this.sub = ctx.createOscillator();
    this.sub.type = 'sine';
    this.sub.frequency.value = 40;
    this.subGain = ctx.createGain();
    this.subGain.gain.value = 0.35;
    this.sub.connect(this.subGain);
    this.subGain.connect(this.engineGain);
    this.sub.start();

    // --- wind / speed noise -------------------------------------------------
    const len = ctx.sampleRate * 2;
    const buf = ctx.createBuffer(1, len, ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < len; i++) d[i] = Math.random() * 2 - 1;
    this.noiseBuf = buf;

    this.windSrc = ctx.createBufferSource();
    this.windSrc.buffer = buf;
    this.windSrc.loop = true;
    this.windFilter = ctx.createBiquadFilter();
    this.windFilter.type = 'bandpass';
    this.windFilter.frequency.value = 900;
    this.windFilter.Q.value = 0.8;
    this.windGain = ctx.createGain();
    this.windGain.gain.value = 0;
    this.windSrc.connect(this.windFilter);
    this.windFilter.connect(this.windGain);
    this.windGain.connect(this.master);
    this.windSrc.start();

    this.ready = true;
    this.setMuted(this.muted);
  }

  setMuted(m) {
    this.muted = m;
    if (!this.ready) return;
    this.master.gain.setTargetAtTime(m ? 0.0001 : 0.5, this.ctx.currentTime, 0.05);
  }

  /**
   * @param {number} speedNorm 0..1 fraction of top speed
   * @param {number} throttle 0..1
   * @param {number} load 0..1 lateral slip / strain
   * @param {number} surge 0..1 tachyon surge amount
   */
  update(speedNorm, throttle, load, surge) {
    if (!this.ready || this.muted) return;
    const t = this.ctx.currentTime;
    const base = 38 + speedNorm * 168 + surge * 60;
    for (const { o, ratio } of this.oscs) o.frequency.setTargetAtTime(base * ratio, t, 0.06);
    this.sub.frequency.setTargetAtTime(base * 0.5, t, 0.08);
    this.engineFilter.frequency.setTargetAtTime(360 + speedNorm * 2600 + surge * 1800, t, 0.07);
    this.engineGain.gain.setTargetAtTime(0.10 + throttle * 0.09 + surge * 0.06, t, 0.08);
    this.windGain.gain.setTargetAtTime(Math.pow(speedNorm, 2.2) * 0.16, t, 0.1);
    this.windFilter.frequency.setTargetAtTime(600 + speedNorm * 2600, t, 0.1);
  }

  _env(node, t, peak, attack, decay) {
    const g = this.ctx.createGain();
    g.gain.setValueAtTime(0.0001, t);
    g.gain.linearRampToValueAtTime(peak, t + attack);
    g.gain.exponentialRampToValueAtTime(0.0001, t + attack + decay);
    node.connect(g);
    g.connect(this.master);
    return g;
  }

  gate(combo = 1) {
    if (!this.ready || this.muted) return;
    const t = this.ctx.currentTime;
    const root = 523.25 * Math.pow(2, Math.min(combo, 12) / 12);
    for (const [mult, delay] of [[1, 0], [1.5, 0.045], [2, 0.09]]) {
      const o = this.ctx.createOscillator();
      o.type = 'triangle';
      o.frequency.setValueAtTime(root * mult, t + delay);
      this._env(o, t + delay, 0.10, 0.006, 0.28);
      o.start(t + delay);
      o.stop(t + delay + 0.34);
    }
  }

  miss() {
    if (!this.ready || this.muted) return;
    const t = this.ctx.currentTime;
    const o = this.ctx.createOscillator();
    o.type = 'sawtooth';
    o.frequency.setValueAtTime(180, t);
    o.frequency.exponentialRampToValueAtTime(60, t + 0.25);
    const f = this.ctx.createBiquadFilter();
    f.type = 'lowpass'; f.frequency.value = 700;
    o.connect(f);
    this._env(f, t, 0.11, 0.005, 0.26);
    o.start(t); o.stop(t + 0.32);
  }

  boost() {
    if (!this.ready || this.muted) return;
    const t = this.ctx.currentTime;
    const src = this.ctx.createBufferSource();
    src.buffer = this.noiseBuf;
    const f = this.ctx.createBiquadFilter();
    f.type = 'bandpass';
    f.frequency.setValueAtTime(300, t);
    f.frequency.exponentialRampToValueAtTime(3200, t + 0.4);
    f.Q.value = 1.4;
    src.connect(f);
    this._env(f, t, 0.22, 0.02, 0.5);
    src.start(t); src.stop(t + 0.6);
  }

  impact() {
    if (!this.ready || this.muted) return;
    const t = this.ctx.currentTime;
    const src = this.ctx.createBufferSource();
    src.buffer = this.noiseBuf;
    const f = this.ctx.createBiquadFilter();
    f.type = 'lowpass'; f.frequency.value = 420; f.Q.value = 1.2;
    src.connect(f);
    this._env(f, t, 0.34, 0.004, 0.3);
    src.start(t); src.stop(t + 0.4);
    const o = this.ctx.createOscillator();
    o.type = 'sine';
    o.frequency.setValueAtTime(120, t);
    o.frequency.exponentialRampToValueAtTime(38, t + 0.3);
    this._env(o, t, 0.3, 0.004, 0.32);
    o.start(t); o.stop(t + 0.4);
  }

  surge() {
    if (!this.ready || this.muted) return;
    const t = this.ctx.currentTime;
    for (const mult of [1, 1.5, 2, 3]) {
      const o = this.ctx.createOscillator();
      o.type = 'sine';
      o.frequency.setValueAtTime(220 * mult, t);
      o.frequency.exponentialRampToValueAtTime(220 * mult * 3, t + 0.7);
      this._env(o, t, 0.06, 0.03, 0.75);
      o.start(t); o.stop(t + 0.9);
    }
    const src = this.ctx.createBufferSource();
    src.buffer = this.noiseBuf;
    const f = this.ctx.createBiquadFilter();
    f.type = 'highpass'; f.frequency.value = 2400;
    src.connect(f);
    this._env(f, t, 0.12, 0.12, 0.8);
    src.start(t); src.stop(t + 1.0);
  }

  beep(freq = 660, dur = 0.16, vol = 0.14) {
    if (!this.ready || this.muted) return;
    const t = this.ctx.currentTime;
    const o = this.ctx.createOscillator();
    o.type = 'square';
    o.frequency.value = freq;
    this._env(o, t, vol, 0.005, dur);
    o.start(t); o.stop(t + dur + 0.05);
  }
}
