const REFRESH_MS = 2000;
const DEFAULT_COUNT = 10;
const MIN_COUNT = 1;
const MAX_COUNT = 1440;

function wantedCount() {
    const input = document.getElementById('count');
    const value = Number.parseInt(input.value, 10);
    const count = Number.isNaN(value)
        ? DEFAULT_COUNT
        : Math.min(Math.max(value, MIN_COUNT), MAX_COUNT);

    // Show what is actually requested, also when the typed value was clamped.
    input.value = count;
    return count;
}

function buildRow(measurement) {
    const row = document.createElement('tr');
    for (const value of [measurement.temperature, measurement.pressure, measurement.humidity]) {
        const cell = document.createElement('td');
        cell.textContent = value.toFixed(2);
        row.appendChild(cell);
    }
    return row;
}

async function refresh() {
    const tableBody = document.getElementById('measurements');
    const status = document.getElementById('status');
    const count = wantedCount();

    try {
        const response = await fetch(`/api/measurements?n=${count}`);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }

        const measurements = await response.json();
        // Newest first.
        tableBody.replaceChildren(...measurements.reverse().map(buildRow));
        status.textContent = `Laatste update: ${new Date().toLocaleTimeString('nl-NL')} (${measurements.length} van ${count} metingen)`;
    } catch (error) {
        status.textContent = `Metingen ophalen mislukt: ${error.message}`;
    }
}

document.getElementById('count').addEventListener('change', refresh);

refresh();
setInterval(refresh, REFRESH_MS);
