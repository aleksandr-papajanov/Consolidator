const fs = require('fs');
const root = __dirname;
const reference = JSON.parse(fs.readFileSync(root + '/short-reference-observations.json'));
function basis(x, degree)
{
    const values = [1, x];
    for (let index = 2; index <= degree; ++index)
    {
        values.push(2 * x * values[index - 1] - values[index - 2]);
    }
    return values.slice(0, degree + 1);
}
function solve(matrix, rhs)
{
    const count = rhs.length;
    const rows = matrix.map((row, index) => [...row, rhs[index]]);
    for (let column = 0; column < count; ++column)
    {
        let pivot = column;
        for (let row = column + 1; row < count; ++row)
        {
            if (Math.abs(rows[row][column]) > Math.abs(rows[pivot][column])) pivot = row;
        }
        [rows[column], rows[pivot]] = [rows[pivot], rows[column]];
        const scale = rows[column][column];
        for (let index = column; index <= count; ++index) rows[column][index] /= scale;
        for (let row = 0; row < count; ++row)
        {
            if (row === column) continue;
            const factor = rows[row][column];
            for (let index = column; index <= count; ++index)
            {
                rows[row][index] -= factor * rows[column][index];
            }
        }
    }
    return rows.map(row => row[count]);
}
function fit(rows, degree)
{
    const count = degree + 1;
    const matrix = Array.from({ length: count }, () => Array(count).fill(0));
    const rhs = Array(count).fill(0);
    for (const row of rows)
    {
        const vector = basis(row.x, degree), weight = row.weight || 1;
        for (let i = 0; i < count; ++i)
        {
            rhs[i] += weight * vector[i] * row.y;
            for (let j = 0; j < count; ++j) matrix[i][j] += weight * vector[i] * vector[j];
        }
    }
    return solve(matrix, rhs);
}
function evaluate(coefficients, x)
{
    const vector = basis(x, coefficients.length - 1);
    return coefficients.reduce((sum, value, index) => sum + value * vector[index], 0);
}
const gains = reference.filter(row => row.reduction === 0).map(row => ({
    x: 2 * row.gainNormalized - 1, y: row.cycles[0].gainDb.at(-1),
    weight: [0, 40, 100].includes(row.gain) ? 4 : 1
}));
for (const degree of [5, 7, 9, 11, 13])
{
    const coefficients = fit(gains, degree);
    const errors = gains.map(row => evaluate(coefficients, row.x) - row.y);
    const steps = Array.from({ length: 1000 }, (_, index) =>
        evaluate(coefficients, (index + 1) / 500 - 1) - evaluate(coefficients, index / 500 - 1));
    console.log({ degree, max: Math.max(...errors.map(Math.abs)), minStep: Math.min(...steps), coefficients });
    if (degree === 9)
    {
        fs.writeFileSync(root + '/short-gain-fit.json', JSON.stringify({ coefficients, errors }, null, 2));
    }
}
module.exports = { basis, solve, fit, evaluate };
