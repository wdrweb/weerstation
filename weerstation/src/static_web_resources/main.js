
const demo = new URLSearchParams(location.search).get("demo") === "1";
const grafieken = {};
let minuten = 15;
let bezig = false;

function tijd(t) {
    return new Date(t * 1000).toLocaleString("nl-NL");
}

function grafiek(id, data, naam, kleur) {
    if (typeof Chart === "undefined") return;

    const labels = data.map(m => tijd(m.timestamp));
    const waarden = data.map(m => m[naam]);

    if (grafieken[id]) {
        grafieken[id].data.labels = labels;
        grafieken[id].data.datasets[0].data = waarden;
        grafieken[id].update("none");
    } else {
        grafieken[id] = new Chart(document.getElementById(id), {
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

    const vanaf = Date.now() / 1000 - minuten * 60;
    const periode = data.filter(m => m.timestamp >= vanaf);

    const tabel = document.getElementById("history");
    tabel.replaceChildren();

    periode.slice().reverse().forEach(m => {
        const rij = tabel.insertRow();
        [tijd(m.timestamp), m.temperature.toFixed(1),
         m.humidity.toFixed(1), m.pressure.toFixed(1)]
        .forEach(waarde => rij.insertCell().textContent = waarde);
    });

    grafiek("temperatureChart", periode, "temperature", "red");
    grafiek("humidityChart", periode, "humidity", "blue");
    grafiek("pressureChart", periode, "pressure", "green");

    return periode.length;
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
    if (bezig) return;
    bezig = true;

    const status = document.getElementById("status");

    try {
        let data;

        if (demo) {
            data = testData(minuten + 1);
        } else {
            const antwoord = await fetch(
                "/api/measurements?n=" + Math.min(minuten + 1, 1440)
            );
            if (!antwoord.ok) throw new Error("HTTP " + antwoord.status);
            data = await antwoord.json();
        }

        if (!Array.isArray(data)) throw new Error("Ongeldige gegevens");

        if (data.length === 0) {
            status.textContent = "Nog geen metingen";
        } else {
            const aantal = tonen(data);
            status.textContent = demo
                ? "Teststand met voorbeeldmetingen"
                : "Verbonden - " + new Date().toLocaleTimeString("nl-NL");

            if (aantal === 0) status.textContent += " - Geen recente metingen";
            if (typeof Chart === "undefined") {
                status.textContent += " - Grafieken niet geladen";
            }
        }
    } catch (fout) {
        status.textContent = "Fout: " + fout.message;
    } finally {
        bezig = false;
    }
}

document.querySelectorAll(".periode").forEach(knop => {
    knop.onclick = function() {
        minuten = Number(knop.dataset.min);

        document.querySelectorAll(".periode").forEach(k => {
            k.classList.remove("actief");
        });

        knop.classList.add("actief");
        laden();
    };
});

document.getElementById("refresh").onclick = laden;

laden();
setInterval(laden, 5000);
