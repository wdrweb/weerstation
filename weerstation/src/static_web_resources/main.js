
const demo = new URLSearchParams(location.search).has("demo");
const charts = {};

function tijd(t) {
    return new Date(t * 1000).toLocaleString("nl-NL");
}

function grafiek(id, data, naam, kleur) {
    const labels = data.map(m => tijd(m.timestamp));
    const waarden = data.map(m => m[naam]);

    if (charts[id]) {
        charts[id].data.labels = labels;
        charts[id].data.datasets[0].data = waarden;
        charts[id].update();
    } else {
        charts[id] = new Chart(document.getElementById(id), {
            type: "line",
            data: {
                labels: labels,
                datasets: [{
                    label: naam,
                    data: waarden,
                    borderColor: kleur,
                    pointRadius: 2
                }]
            },
            options: {
                animation: false,
                scales: { x: { ticks: { maxTicksLimit: 5 } } }
            }
        });
    }
}

function tonen(data) {
    data.sort((a, b) => a.timestamp - b.timestamp);
    const laatste = data[data.length - 1];

    document.getElementById("temperature").textContent =
        laatste.temperature.toFixed(1) + " °C";
    document.getElementById("humidity").textContent =
        laatste.humidity.toFixed(1) + " %";
    document.getElementById("pressure").textContent =
        laatste.pressure.toFixed(1) + " kPa";
    document.getElementById("lastMeasurement").textContent =
        "Laatste meting: " + tijd(laatste.timestamp);

    const tabel = document.getElementById("history");
    tabel.innerHTML = "";

    data.slice().reverse().forEach(m => {
        const rij = document.createElement("tr");

        [
            tijd(m.timestamp),
            m.temperature.toFixed(1),
            m.humidity.toFixed(1),
            m.pressure.toFixed(1)
        ].forEach(waarde => {
            const cel = document.createElement("td");
            cel.textContent = waarde;
            rij.appendChild(cel);
        });

        tabel.appendChild(rij);
    });

    grafiek("temperatureChart", data, "temperature", "red");
    grafiek("humidityChart", data, "humidity", "blue");
    grafiek("pressureChart", data, "pressure", "green");
}

function testData(aantal) {
    const data = [];
    const nu = Math.floor(Date.now() / 1000);

    for (let i = 0; i < aantal; i++) {
        data.push({
            temperature: 21 + Math.sin(i / 4) * 2,
            humidity: 55 + Math.cos(i / 5) * 8,
            pressure: 101.3 + Math.sin(i / 6) * 0.5,
            timestamp: nu - (aantal - i - 1) * 60
        });
    }
    return data;
}

async function laden() {
    const invoer = document.getElementById("count");
    let aantal = Number(invoer.value);

    if (!Number.isInteger(aantal) || aantal < 1) aantal = 10;
    if (aantal > 1440) aantal = 1440;
    invoer.value = aantal;

    try {
        let data;

        if (demo) {
            data = testData(aantal);
        } else {
            const antwoord = await fetch("/api/measurements?n=" + aantal);
            if (!antwoord.ok) throw new Error("HTTP " + antwoord.status);
            data = await antwoord.json();
        }

        if (!Array.isArray(data)) throw new Error("Ongeldige gegevens");

        if (data.length === 0) {
            document.getElementById("status").textContent =
                "Nog geen metingen";
            return;
        }

        tonen(data);
        document.getElementById("status").textContent =
            demo ? "Teststand met voorbeeldmetingen" :
            "Verbonden - bijgewerkt om " +
            new Date().toLocaleTimeString("nl-NL");

    } catch (fout) {
        document.getElementById("status").textContent =
            "Fout bij ophalen: " + fout.message;
    }
}

document.getElementById("refresh").onclick = laden;
document.getElementById("count").onchange = laden;

laden();
setInterval(laden, 5000);
