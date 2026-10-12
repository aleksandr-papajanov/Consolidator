const fs = require('fs');
const source = fs.readFileSync(__dirname + '/fit.js', 'utf8');
eval(source.slice(0, source.indexOf('const root=__dirname;')).replace('function minimize', 'global.minimize = function'));
const reference = JSON.parse(fs.readFileSync(__dirname + '/short-reference-observations.json'));
const base = reference.find(row => row.id === 'gain40-pr0');
const curves = reference.filter(row => row.gain === 40 && row.reduction >= 42);
function objective(p, details = false)
{
    const [fastCharge, slowCharge, fraction, fastRecovery, slowRecovery, floor, targetScale] = p;
    if (fastCharge < .0001 || fastCharge > .1 || slowCharge < fastCharge || slowCharge > 1 ||
        fraction < .03 || fraction > .6 || fastRecovery < .1 || fastRecovery > 15 ||
        slowRecovery < fastRecovery || slowRecovery > 80 || floor < .01 || floor > 10 ||
        targetScale < .3 || targetScale > 100) return 1e8;
    let loss = 0, count = 0;
    const results = [];
    for (const curve of curves)
    {
        const lowTarget = Math.max(0, base.cycles[0].gainDb.at(-1) - curve.cycles[0].gainDb.at(-1));
        const highTarget = Math.max(0, base.stationary.at(-1).gainDb - curve.stationary.at(-1).gainDb);
        const weight = 1 / Math.sqrt(2 + highTarget);
        let fast = 0, slow = 0;
        for (let rowIndex = 0; rowIndex < curve.cycles.length; ++rowIndex)
        {
            const row = curve.cycles[rowIndex], target = row.kind === 'high' ? highTarget : lowTarget;
            const duration = row.kind === 'high' ? 2 : 1;
            let time = 0;
            const predictions = [];
            function advance(end)
            {
                while (time < end - 1e-9)
                {
                    const step = Math.min(end - time, time < .05 ? .0005 : time < .3 ? .003 : .02);
                    const drive = Math.sqrt((target + .1) / (target + 22));
                    const recovery = 1 + target / targetScale;
                    const fastRate = target >= fast ? drive / fastCharge : (fast + floor) * recovery / fastRecovery;
                    const slowRate = target >= slow ? drive / slowCharge : (slow + floor) * recovery * recovery / slowRecovery;
                    fast += (target - fast) * (1 - Math.exp(-step * fastRate));
                    slow += (target - slow) * (1 - Math.exp(-step * slowRate));
                    time += step;
                }
            }
            row.times.forEach((point, index) =>
            {
                advance(point + .0015);
                const observed = base.cycles[rowIndex].gainDb[index] - row.gainDb[index];
                const predicted = fast * (1 - fraction) + slow * fraction;
                predictions.push(predicted);
                const historyWeight = row.kind === 'low' && row.cycle > 0 ? 3 : 1;
                loss += (predicted - observed) ** 2 * weight * (point < .01 ? .4 : 1) * historyWeight;
                count++;
            });
            advance(duration);
            if (details && curve.reduction === 46)
            {
                results.push({ kind: row.kind, cycle: row.cycle, target, predicted: predictions,
                    observed: row.gainDb.map((value, index) => base.cycles[rowIndex].gainDb[index] - value) });
            }
        }
    }
    return details ? results : loss / count;
}
const fit = minimize(p => objective(p), [.0031, .06, .22, 2, 20, 2, 6], [.001, .02, .06, .5, 4, .5, 2], 3000);
fs.writeFileSync(__dirname + '/short-dynamics-fit.json', JSON.stringify({ fit, details: objective(fit.x, true) }, null, 2));
console.log(fit);
