const fs = require('fs');
const source = fs.readFileSync(__dirname + '/fit.js', 'utf8');
eval(source.slice(0, source.indexOf('const root=__dirname;')).replace('function minimize', 'global.minimize = function'));
const scales = JSON.parse(fs.readFileSync(__dirname + '/reference-knob-scale.json'));
for (const control of scales)
{
    const pivot = control.name === 'Gain' ? .35 : .4;
    const height = pivot * 100;
    const curve = (x, low, high) => x <= pivot
        ? height * Math.expm1(-low * x) / Math.expm1(-low * pivot)
        : height + (100 - height) * Math.expm1(high * (x - pivot)) / Math.expm1(high * (1 - pivot));
    const fit = minimize(p => control.scale.reduce((sum, row) => sum + (curve(row.normalized, ...p) - parseFloat(row.text)) ** 2, 0), [6, 4.5], [.2, .2], 1200);
    const errors = control.scale.map(row => curve(row.normalized, ...fit.x) - parseFloat(row.text));
    console.log(control.name, fit, 'max', Math.max(...errors.map(Math.abs)));
    console.log([0, .1, .35, .4, .535, .598, .75, 1].map(x => [x, curve(x, ...fit.x)]));
}
