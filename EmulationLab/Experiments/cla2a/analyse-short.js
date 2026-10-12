const fs = require('fs');
const root = __dirname;
const segments = JSON.parse(fs.readFileSync(root + '/short-segments.json')).segments;
const manifest = JSON.parse(fs.readFileSync(root + '/short-manifest.json'));
function wave(file)
{
    const buffer = fs.readFileSync(file);
    let data, rate;
    for (let offset = 12; offset + 8 <= buffer.length;)
    {
        const length = buffer.readUInt32LE(offset + 4);
        const tag = buffer.toString('ascii', offset, offset + 4);
        if (tag === 'fmt ') rate = buffer.readUInt32LE(offset + 12);
        if (tag === 'data') data = offset + 8;
        offset += 8 + length + length % 2;
    }
    return { buffer, data, rate };
}
function amplitude(w, start, duration)
{
    const first = Math.round(start * w.rate), count = Math.round(duration * w.rate);
    let cc = 0, ss = 0, cs = 0, csum = 0, ssum = 0, yc = 0, ys = 0, ysum = 0;
    for (let index = first; index < first + count; ++index)
    {
        const phase = 2 * Math.PI * 998.6 * index / w.rate;
        const c = Math.cos(phase), s = Math.sin(phase);
        const y = w.buffer.readFloatLE(w.data + index * 8);
        cc += c * c; ss += s * s; cs += c * s; csum += c; ssum += s;
        yc += y * c; ys += y * s; ysum += y;
    }
    // Eliminate the intercept: filter transients must not masquerade as amplitude.
    cc -= csum * csum / count; ss -= ssum * ssum / count; cs -= csum * ssum / count;
    yc -= ysum * csum / count; ys -= ysum * ssum / count;
    const determinant = cc * ss - cs * cs;
    return Math.hypot((yc * ss - ys * cs) / determinant, (ys * cc - yc * cs) / determinant);
}
const set = process.argv[2];
const results = [];
for (const condition of manifest.conditions)
{
    const w = wave(root + '/' + set + '/' + condition.id + '.wav');
    const readback = JSON.parse(fs.readFileSync(root + '/' + set + '/' + condition.id + '.parameters.json'));
    const parameter = name => readback.parameters.find(row => row.name === name);
    const result = { id: condition.id, gain: parseFloat(parameter('Gain').text),
        reduction: parseFloat(parameter('Peak Reduction').text), gainNormalized: parameter('Gain').normalized,
        reductionNormalized: parameter('Peak Reduction').normalized, stationary: [], cycles: [] };
    for (const segment of segments)
    {
        const start = segment.startFrame / w.rate;
        const gainDb = (time, duration) => 20 * Math.log10(Math.max(1e-15, amplitude(w, start + time, duration))) - segment.levelDb;
        if (segment.kind === 'stationary')
        {
            result.stationary.push({ input: segment.levelDb, gainDb: gainDb(segment.seconds - 1, 1) });
            continue;
        }
        const times = segment.kind === 'high' ? [.002, .005, .01, .02, .05, .1, .2, .4, .8, 1.5, 1.9]
            : segment.kind === 'release' ? [.002, .005, .01, .02, .05, .1, .2, .4, .6, .9]
            : [.003, .01, .03, .1, .3, .6, .9];
        result.cycles.push({ kind: segment.kind, cycle: segment.cycle, input: segment.levelDb,
            times, gainDb: times.map(time => gainDb(time, .003)) });
    }
    results.push(result);
}
fs.writeFileSync(root + '/' + set + '-observations.json', JSON.stringify(results, null, 2));
console.log(JSON.stringify(results.filter(row => row.id === 'gain40-pr46'), null, 2));
