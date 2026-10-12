const fs = require('fs');
const path = require('path');

const root = __dirname;
const segments = JSON.parse(fs.readFileSync(path.join(root, 'segments.json'))).segments;
const manifest = JSON.parse(fs.readFileSync(path.join(root, 'manifest.json')));

function readWave(file)
{
    const buffer = fs.readFileSync(file);
    let data;
    let sampleRate;
    for (let offset = 12; offset + 8 <= buffer.length;)
    {
        const length = buffer.readUInt32LE(offset + 4);
        const tag = buffer.toString('ascii', offset, offset + 4);
        if (tag === 'fmt ')
        {
            sampleRate = buffer.readUInt32LE(offset + 12);
        }
        if (tag === 'data')
        {
            data = offset + 8;
        }
        offset += 8 + length + length % 2;
    }
    return { buffer, data, sampleRate };
}

function amplitude(wave, startSeconds, durationSeconds, frequency)
{
    const first = Math.round(startSeconds * wave.sampleRate);
    const count = Math.round(durationSeconds * wave.sampleRate);
    let cc = 0, ss = 0, cs = 0, csum = 0, ssum = 0, yc = 0, ys = 0, ysum = 0;
    for (let index = first; index < first + count; ++index)
    {
        const phase = 2 * Math.PI * frequency * index / wave.sampleRate;
        const c = Math.cos(phase);
        const s = Math.sin(phase);
        const value = wave.buffer.readFloatLE(wave.data + index * 8);
        cc += c * c;
        ss += s * s;
        cs += c * s;
        csum += c;
        ssum += s;
        yc += value * c;
        ys += value * s;
        ysum += value;
    }
    // Include a DC intercept so settling does not bias the estimated fundamental.
    cc -= csum * csum / count;
    ss -= ssum * ssum / count;
    cs -= csum * ssum / count;
    yc -= ysum * csum / count;
    ys -= ysum * ssum / count;
    const determinant = cc * ss - cs * cs;
    return Math.hypot((yc * ss - ys * cs) / determinant, (ys * cc - yc * cs) / determinant);
}

const directory = process.argv[2];
const observations = [];
for (const condition of manifest.conditions)
{
    const wave = readWave(path.join(root, directory, condition.id + '.wav'));
    const readback = JSON.parse(fs.readFileSync(path.join(root, directory, condition.id + '.parameters.json')));
    const result = { id: condition.id, parameters: readback.parameters, segments: [] };
    for (const segment of segments)
    {
        const start = segment.startFrame / wave.sampleRate;
        const duration = Math.min(1, segment.seconds * 0.4);
        const steadyGainDb = 20 * Math.log10(amplitude(
            wave, start + segment.seconds - duration, duration, segment.frequency)) - segment.levelDb;
        const row = { kind: segment.kind, levelDb: segment.levelDb, frequency: segment.frequency, steadyGainDb };
        if (segment.kind === 'high' || segment.kind === 'release' || segment.kind === 'low')
        {
            row.times = segment.kind === 'release'
                ? [.003, .01, .03, .1, .2, .4, .8, 1.5, 3, 5, 7]
                : [.003, .006, .01, .02, .04, .08, .15, .3, .6, 1, 1.8];
            row.gainDb = row.times.map(time => 20 * Math.log10(amplitude(
                wave, start + time, .003, segment.frequency)) - segment.levelDb);
        }
        result.segments.push(row);
    }
    observations.push(result);
}
fs.writeFileSync(path.join(root, directory + '-observations.json'), JSON.stringify(observations, null, 2));
console.log('Analysed conditions: ' + observations.length);
