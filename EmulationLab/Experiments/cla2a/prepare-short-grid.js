const fs = require('fs');
const path = require('path');
const sampleRate = 44100;
const segments = [];
for (let cycle = 0; cycle < 3; ++cycle)
{
    for (const [kind, levelDb, seconds] of [['low', -20, 1], ['high', 0, 2], ['release', -20, 1]])
    {
        segments.push({ kind, levelDb, seconds, cycle, frequency: 998.6 });
    }
}
for (const levelDb of [-48, -36, -24, -12, -6, 0])
{
    segments.push({ kind: 'stationary', levelDb, seconds: 4, frequency: 998.6 });
}
let frames = 0;
for (const segment of segments)
{
    segment.startFrame = frames;
    frames += Math.round(segment.seconds * sampleRate);
}
const buffer = Buffer.alloc(44 + frames * 8);
buffer.write('RIFF'); buffer.writeUInt32LE(buffer.length - 8, 4);
buffer.write('WAVEfmt ', 8); buffer.writeUInt32LE(16, 16);
buffer.writeUInt16LE(3, 20); buffer.writeUInt16LE(2, 22);
buffer.writeUInt32LE(sampleRate, 24); buffer.writeUInt32LE(sampleRate * 8, 28);
buffer.writeUInt16LE(8, 32); buffer.writeUInt16LE(32, 34);
buffer.write('data', 36); buffer.writeUInt32LE(frames * 8, 40);
let phase = 0;
for (const segment of segments)
{
    for (let index = 0; index < segment.seconds * sampleRate; ++index)
    {
        const value = 10 ** (segment.levelDb / 20) * Math.sin(phase);
        const offset = 44 + (segment.startFrame + index) * 8;
        buffer.writeFloatLE(value, offset); buffer.writeFloatLE(value, offset + 4);
        phase = (phase + 2 * Math.PI * segment.frequency / sampleRate) % (2 * Math.PI);
    }
}
const raw = path.resolve(__dirname, '../../RawAudio/Probes/cla2a-short-repeated-9986.wav');
if (!fs.existsSync(raw))
{
    fs.writeFileSync(raw, buffer);
}
const manifest = JSON.parse(fs.readFileSync(__dirname + '/manifest.json'));
manifest.name = 'cla2a-short-repeated-displayed-controls';
const conditions = [];
const add = (gain, reduction) =>
{
    const parameters = { Gain: gain.toFixed(2), 'Peak Reduction': reduction.toFixed(2) };
    conditions.push({ id: `gain${gain}-pr${reduction}`, reference: { parameters },
        candidate: { parameters: { ...parameters, Bypass: 0 } } });
};
for (const gain of [0, 10, 15, 20, 25, 30, 32.28, 35, 38, 40, 42, 45, 50, 55, 60, 65, 70, 80, 90, 100])
{
    add(gain, 0);
}
for (const reduction of [20, 30, 35, 38, 40, 42, 44, 46, 48, 50, 55, 60, 65, 70, 80, 90, 100])
{
    add(40, reduction);
}
for (const gain of [25, 60, 80])
{
    for (const reduction of [40, 60, 80])
    {
        add(gain, reduction);
    }
}
manifest.conditions = conditions;
fs.writeFileSync(__dirname + '/short-manifest.json', JSON.stringify(manifest, null, 2));
fs.writeFileSync(__dirname + '/short-segments.json', JSON.stringify({ sampleRate, segments }, null, 2));
console.log(raw, frames / sampleRate, conditions.length);
