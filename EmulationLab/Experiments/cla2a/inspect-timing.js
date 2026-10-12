const fs = require('fs');
const root = __dirname;
const segments = JSON.parse(fs.readFileSync(root + '/segments.json')).segments.slice(0, 12);
const times = [.002, .005, .01, .02, .05, .1, .2, .5, 1, 2, 3.8];
function wave(file) {
    const b = fs.readFileSync(file); let data, rate;
    for (let o = 12; o + 8 <= b.length;) {
        const n = b.readUInt32LE(o + 4), tag = b.toString('ascii', o, o + 4);
        if (tag === 'fmt ') rate = b.readUInt32LE(o + 12);
        if (tag === 'data') data = o + 8;
        o += 8 + n + n % 2;
    }
    return { b, data, rate };
}
function amplitude(w, seconds, frequency) {
    // Least-squares sine/cosine projection avoids short-window phase bias.
    const first = Math.round(seconds * w.rate), count = Math.round(.003 * w.rate);
    let cc = 0, ss = 0, cs = 0, yc = 0, ys = 0;
    for (let i = first; i < first + count; ++i) {
        const p = 2 * Math.PI * frequency * i / w.rate;
        const c = Math.cos(p), s = Math.sin(p), y = w.b.readFloatLE(w.data + i * 8);
        cc += c * c; ss += s * s; cs += c * s; yc += y * c; ys += y * s;
    }
    const d = cc * ss - cs * cs;
    return Math.hypot((yc * ss - ys * cs) / d, (ys * cc - yc * cs) / d);
}
const set = process.argv[2] || 'reference';
const results = [];
for (const pr of [.4, .5, .6, .7, .8, .9, 1]) {
    const w = wave(root + '/' + set + '/reduction-' + pr + '.wav');
    const rows = segments.map(s => ({ kind: s.kind, input: s.levelDb,
        reduction: times.map(t => +(7.1 + s.levelDb - 20 * Math.log10(amplitude(w, s.startFrame / w.rate + t, s.frequency))).toFixed(3)) }));
    results.push({ pr, times, rows });
}
fs.writeFileSync(root + '/' + set + '-timing.json', JSON.stringify(results, null, 2));
console.log(JSON.stringify(results.filter(r => [.5, .6, .8, 1].includes(r.pr)).map(r => ({ pr: r.pr, times, rows: r.rows.slice(0, 3) })), null, 2));
