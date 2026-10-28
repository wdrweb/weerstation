
const grafieken = {};
let minuten = 15;
let bezig = false;
let zoomHerstellen = false;

function tijd(t) {
    return new Date(t * 1000).toLocaleString("nl-NL");
}

function klok(t) {
    return new Date(t * 1000).toISOString().slice(11, 19);
}

function tijdLabel(m) {
    return m.relative ? "relative " + klok(m.timestamp) : tijd(m.timestamp);
}

function grafiek(id, data, naam, kleur, kleurRelatief) {
    if (typeof Chart === "undefined") return;

    const labels = data.map(tijdLabel);
    const waarden = [
        data.map(m => m.relative ? null : m[naam]),
        data.map(m => m.relative ? m[naam] : null)
    ];

    if (grafieken[id]) {
        grafieken[id].data.labels = labels;
        grafieken[id].data.datasets[0].data = waarden[0];
        grafieken[id].data.datasets[1].data = waarden[1];
        grafieken[id].update("none");
        return;
    }

    grafieken[id] = new Chart(document.getElementById(id), {
        type: "line",
        data: {
            labels: labels,
            datasets: [{
                label: naam + " (gesynchroniseerd)",
                data: waarden[0],
                borderColor: kleur,
                pointRadius: 2,
                spanGaps: false
            }, {
                label: naam + " (relatief)",
                data: waarden[1],
                borderColor: kleurRelatief,
                borderDash: [6, 4],
                pointRadius: 2,
                spanGaps: false
            }]
        },
        options: {
            animation: false,
            scales: {
                x: {
                    ticks: {
                        autoSkip: true,
                        maxRotation: 0,
                        autoSkipPadding: 24
                    }
                }
            },
            plugins: {
                zoom: {
                    pan: { enabled: true, mode: "x" },
                    zoom: {
                        wheel: { enabled: true },
                        pinch: { enabled: true },
                        mode: "x"
                    }
                }
            }
        }
    });
}

function tonen(data) {
    //write order is already oldest to newest; sorting would interleave the time bases
    const laatste = data[data.length - 1];
    const relatief = laatste.relative;

    document.getElementById("temperature").textContent =
        laatste.temperature.toFixed(1) + " °C";
    document.getElementById("humidity").textContent =
        laatste.humidity.toFixed(1) + " %";
    document.getElementById("pressure").textContent =
        laatste.pressure.toFixed(1) + " kPa";
    document.getElementById("lastMeasurement").textContent =
        "Laatste meting: " + tijdLabel(laatste) +
        (relatief ? " (tijd niet gesynchroniseerd)" : "");

    // Periodefilter staat uit: de API levert al n metingen aan.
    // const vanaf = Date.now() / 1000 - minuten * 60;
    // const periode = data.filter(m => m.timestamp >= vanaf);
    const periode = data;

    const tabel = document.getElementById("history");
    tabel.replaceChildren();

    periode.slice().reverse().forEach(m => {
        const rij = tabel.insertRow();
        if (m.relative) rij.className = "relatief";
        [tijdLabel(m), m.temperature.toFixed(1),
         m.humidity.toFixed(1), m.pressure.toFixed(1)]
        .forEach(waarde => rij.insertCell().textContent = waarde);
    });

    grafiek("temperatureChart", periode, "temperature", "red",
            "rgba(255, 0, 0, 0.45)");
    grafiek("humidityChart", periode, "humidity", "blue",
            "rgba(0, 0, 255, 0.45)");
    grafiek("pressureChart", periode, "pressure", "green",
            "rgba(0, 128, 0, 0.45)");

    if (zoomHerstellen) {
        Object.values(grafieken).forEach(g => {
            if (typeof g.resetZoom === "function") g.resetZoom();
        });
        zoomHerstellen = false;
    }
}

async function laden() {
    if (bezig) return;
    bezig = true;

    const status = document.getElementById("status");

    try {
        const antwoord = await fetch(
            "/api/measurements?n=" + Math.min(minuten + 1, 1440)
        );
        if (!antwoord.ok) throw new Error("HTTP " + antwoord.status);
        const data = await antwoord.json();

        if (!Array.isArray(data)) throw new Error("Ongeldige gegevens");

        if (data.length === 0) {
            status.textContent = "Nog geen metingen";
        } else {
            tonen(data);
            status.textContent =
                "Verbonden - " + new Date().toLocaleTimeString("nl-NL");

            if (data[data.length - 1].relative) {
                status.textContent += " - Tijd niet gesynchroniseerd";
            }
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
        zoomHerstellen = true;
        laden();
    };
});

document.getElementById("resetZoom").onclick = function() {
    Object.values(grafieken).forEach(g => {
        if (typeof g.resetZoom === "function") g.resetZoom();
    });
};

document.getElementById("refresh").onclick = laden;

laden();
setInterval(laden, 5000);
