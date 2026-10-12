const fs = require('fs');
const root = __dirname;
const source = fs.readFileSync(root + '/fit.js', 'utf8');
eval(source.slice(0, source.indexOf('const root=__dirname;')).replace('function minimize', 'global.minimize = function'));
const reference = JSON.parse(fs.readFileSync(root + '/short-reference-observations.json'));
const candidate = JSON.parse(fs.readFileSync(root + '/short-memory-observations.json'));
const neutral = candidate.find(row => row.id === 'gain40-pr0');
const dynamic = JSON.parse(fs.readFileSync(root + '/short-dynamics-fit.json')).fit.x;
const [fa, sa, fraction, fr, sr, floor, targetScale] = dynamic;
// Offline equilibrium identification, not another audio processor: phase-averaged
// detector power and zero mean change of each optical memory predict stationary gain.
const bins = 64, sums = Array(bins).fill(0), counts = Array(bins).fill(0), rate = 44100;
const attack = 1 - Math.exp(-1 / (.0001 * rate));
const release = 1 - Math.exp(-1 / (.0003 * rate));
let power = 0;
for (let index = 0; index < rate * 2; ++index)
{
    const phase = (998.6 * index / rate) % 1;
    const input = Math.sin(2 * Math.PI * phase) ** 2;
    power += (input > power ? attack : release) * (input - power);
    if (index > rate / 10)
    {
        const bin = Math.floor(phase * bins);
        sums[bin] += power; counts[bin]++;
    }
}
const powersDb = sums.map((sum, index) => 10 * Math.log10(sum / counts[index]));
function threshold(p, knob)
{
    let value = p[0] + 1.69 * Math.max(40 - knob, 0);
    for (let index = 0; index < 6; ++index)
    {
        value -= p[index + 1] * Math.max(0, Math.min(10, knob - 40 - 10 * index));
    }
    return value;
}
function reduction(p, t, input)
{
    const knee = p[7], slope = p[8], deepStrength = p[9];
    const targets = powersDb.map(powerDb =>
    {
        const over = input + powerDb - t;
        let value = over < -knee / 2 ? 0 : over > knee / 2 ? slope * over
            : slope * (over + knee / 2) ** 2 / (2 * knee);
        const excess = Math.max(value - 25, 0);
        value -= deepStrength * excess * excess / (excess + 3);
        return value;
    });
    const charge = targets.map(target => Math.sqrt((target + .1) / (target + 22)));
    const recovery = targets.map(target => 1 + target / targetScale);
    function equilibrium(slow)
    {
        let low = Math.min(...targets), high = Math.max(...targets);
        for (let iteration = 0; iteration < 14; ++iteration)
        {
            const memory = (low + high) / 2;
            let delta = 0;
            for (let bin = 0; bin < bins; ++bin)
            {
                const k = targets[bin] >= memory ? charge[bin] / (slow ? sa : fa)
                    : (memory + floor) * recovery[bin] * (slow ? recovery[bin] : 1) / (slow ? sr : fr);
                const step = k / rate;
                delta += step / (1 + step) * (targets[bin] - memory);
            }
            if (delta > 0) low = memory;
            else high = memory;
        }
        return (low + high) / 2;
    }
    return equilibrium(false) * (1 - fraction) + equilibrium(true) * fraction;
}
const rows = reference.filter(row => row.gain === 40 && row.reduction >= 38).flatMap(row =>
    row.stationary.filter(point => point.input >= -24).map(point => ({ knob: row.reduction, input: point.input,
        target: Math.max(0, neutral.stationary.find(n => n.input === point.input).gainDb - point.gainDb) })));
function loss(p)
{
    if (p[0] < -10 || p[0] > 5 || p.slice(1, 7).some(value => value < .01 || value > 3) ||
        p[7] < .1 || p[7] > 15 || p[8] < .5 || p[8] > .95 || p[9] < 0 || p[9] > .6) return 1e8;
    return rows.reduce((sum, row) => sum + (reduction(p, threshold(p, row.knob), row.input) - row.target) ** 2 *
        (row.knob === 46 ? 3 : 1), 0) / rows.length;
}
const fit = minimize(loss, [-3.5, 1.95, .63, .63, .47, .31, .31, 5, .75, .18],
    [.5, .05, .05, .05, .05, .03, .03, 1, .01, .03], 2200);
const errors = rows.map(row => ({ ...row, error: reduction(fit.x, threshold(fit.x, row.knob), row.input) - row.target }));
fs.writeFileSync(root + '/short-threshold-fit.json', JSON.stringify({ fit, errors }, null, 2));
console.log(fit, 'max error', Math.max(...errors.map(row => Math.abs(row.error))));
