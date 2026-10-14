
const REFRESH_TIME = 5000;

// Teststand aanzetten met ?demo=1 achter het adres
const DEMO_MODE = new URLSearchParams(
    window.location.search
).get("demo") === "1";

let loading = false;

// Controleren hoeveel metingen de gebruiker wil
function getCount() {
    const input = document.getElementById("count");
    let count = Number(input.value);

    if (!Number.isInteger(count) || count < 1) {
        count = 10;
    }

    if (count > 1440) {
        count = 1440;
    }

    input.value = count;
    return count;
}

// Tijd omzetten naar een leesbare datum
function formatTime(timestamp) {
    const date = new Date(timestamp * 1000);
    return date.toLocaleString("nl-NL");
}

// Waarden bovenaan de website tonen
function showCurrentMeasurements(data) {
    const latest = data[data.length - 1];

    document.getElementById("temperature").textContent =
        latest.temperature.toFixed(1) + " °C";

    document.getElementById("humidity").textContent =
        latest.humidity.toFixed(1) + " %";

    document.getElementById("pressure").textContent =
        latest.pressure.toFixed(1) + " kPa";

    document.getElementById("lastMeasurement").textContent =
        "Laatste meting: " + formatTime(latest.timestamp);
}

// Een cel toevoegen aan een tabelrij
function addCell(row, value) {
    const cell = document.createElement("td");
    cell.textContent = value;
    row.appendChild(cell);
}

// Meetgeschiedenis tonen
function showHistory(data) {
    const table = document.getElementById("measurements");

    // Oude rijen verwijderen
    table.replaceChildren();

    // Nieuwste metingen eerst
    const history = data.slice().reverse();

    history.forEach(function(measurement) {
        const row = document.createElement("tr");

        addCell(row, formatTime(measurement.timestamp));
        addCell(row, measurement.temperature.toFixed(1));
        addCell(row, measurement.humidity.toFixed(1));
        addCell(row, measurement.pressure.toFixed(1));

        table.appendChild(row);
    });
}

// Een grafiek tekenen op canvas
function drawChart(canvasId, data, property, color) {
    const canvas = document.getElementById(canvasId);
    const ctx = canvas.getContext("2d");

    canvas.width = 800;
    canvas.height = 240;

    const left = 55;
    const right = 780;
    const top = 20;
    const bottom = 190;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    if (data.length === 0) {
        return;
    }

    // De waarden ophalen
    const values = data.map(function(item) {
        return item[property];
    });

    let min = Math.min(...values);
    let max = Math.max(...values);

    // Extra ruimte onder en boven de lijn
    let difference = max - min;

    if (difference === 0) {
        difference = 2;
    }

    min = min - difference * 0.1;
    max = max + difference * 0.1;

    // Horizontale lijnen met waarden
    for (let i = 0; i <= 4; i++) {
        const y = top + i * (bottom - top) / 4;

        ctx.beginPath();
        ctx.strokeStyle = "#dddddd";
        ctx.lineWidth = 1;
        ctx.moveTo(left, y);
        ctx.lineTo(right, y);
        ctx.stroke();

        const value = max - i * (max - min) / 4;

        ctx.fillStyle = "#555555";
        ctx.font = "12px Arial";
        ctx.fillText(value.toFixed(1), 5, y + 4);
    }

    // De meetpunten verbinden
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;

    values.forEach(function(value, index) {
        const x = left + index * (right - left) /
            Math.max(values.length - 1, 1);

        const y = bottom - (value - min) *
            (bottom - top) / (max - min);

        if (index === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    });

    ctx.stroke();

    // Eerste en laatste tijdstip onder de grafiek
    ctx.fillStyle = "#555555";
    ctx.font = "12px Arial";

    const first = new Date(data[0].timestamp * 1000);
    const last = new Date(
        data[data.length - 1].timestamp * 1000
    );

    ctx.textAlign = "left";
    ctx.fillText(
        first.toLocaleTimeString("nl-NL"),
        left,
        215
    );

    ctx.textAlign = "right";
    ctx.fillText(
        last.toLocaleTimeString("nl-NL"),
        right,
        215
    );

    ctx.textAlign = "left";
}

// Alle grafieken tekenen
function showCharts(data) {
    drawChart(
        "temperatureChart",
        data,
        "temperature",
        "#e74c3c"
    );

    drawChart(
        "humidityChart",
        data,
        "humidity",
        "#2980b9"
    );

    drawChart(
        "pressureChart",
        data,
        "pressure",
        "#27ae60"
    );
}

// Voorbeeldmetingen maken om lokaal te testen
function createDemoData(count) {
    const data = [];
    const now = Math.floor(Date.now() / 1000);

    for (let i = 0; i < count; i++) {
        const number = i + 1;

        data.push({
            temperature: 21 + Math.sin(number / 4) * 2,
            humidity: 55 + Math.cos(number / 5) * 8,
            pressure: 101.3 + Math.sin(number / 6) * 0.5,
            timestamp: now - (count - 1 - i) * 60
        });
    }

    return data;
}

// Controleren of de ontvangen gegevens kloppen
function validMeasurements(data) {
    if (!Array.isArray(data)) {
        return false;
    }

    return data.every(function(item) {
        return Number.isFinite(item.temperature) &&
               Number.isFinite(item.humidity) &&
               Number.isFinite(item.pressure) &&
               Number.isFinite(item.timestamp);
    });
}

// Ontvangen metingen verwerken
function displayMeasurements(data) {
    const status = document.getElementById("status");

    if (!validMeasurements(data)) {
        throw new Error("Ongeldige meetgegevens ontvangen");
    }

    if (data.length === 0) {
        status.textContent =
            "Er zijn nog geen metingen beschikbaar.";
        return;
    }

    // Van oud naar nieuw sorteren
    data.sort(function(a, b) {
        return a.timestamp - b.timestamp;
    });

    showCurrentMeasurements(data);
    showHistory(data);
    showCharts(data);

    if (DEMO_MODE) {
        status.textContent =
            "TESTSTAND - Dit zijn voorbeeldmetingen, geen echte data.";
    } else {
        status.textContent =
            "Verbonden met het weerstation";
    }

    document.getElementById("updateTime").textContent =
        "Bijgewerkt om " +
        new Date().toLocaleTimeString("nl-NL");
}

// Metingen ophalen
function loadMeasurements() {
    if (loading) {
        return;
    }

    loading = true;

    const count = getCount();
    const status = document.getElementById("status");

    // Lokaal testen zonder ESP32
    if (DEMO_MODE) {
        displayMeasurements(createDemoData(count));
        loading = false;
        return;
    }

    // Echte metingen ophalen van het weerstation
    fetch("/api/measurements?n=" + count)
        .then(function(response) {
            if (!response.ok) {
                throw new Error("HTTP " + response.status);
            }

            return response.json();
        })
        .then(function(data) {
            displayMeasurements(data);
        })
        .catch(function(error) {
            status.textContent =
                "Kan geen metingen ophalen: " + error.message;
        })
        .finally(function() {
            loading = false;
        });
}

// Vernieuwen bij wijzigen van het aantal
document.getElementById("count").addEventListener(
    "change",
    loadMeasurements
);

// Vernieuwknop
document.getElementById("refreshButton").addEventListener(
    "click",
    loadMeasurements
);

// Eerste keer gegevens ophalen
loadMeasurements();

// Daarna elke 5 seconden
setInterval(loadMeasurements, REFRESH_TIME);
