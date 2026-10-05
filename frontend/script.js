const API = "http://localhost:8080";

const itemsDiv = document.getElementById("items");
const pathbar = document.getElementById("pathbar");
const statusBar = document.getElementById("status");

async function callServer(route, name) {
    let url = API + route;
    if (name) {
        url += "?name=" + encodeURIComponent(name);
    }

    try {
        const res = await fetch(url);
        const data = await res.json();
        showFolder(data);
    } catch (err) {
        setStatus("Cannot connect to server. Is server.exe running?", true);
    }
}

function showFolder(data) {
    pathbar.textContent = data.path;
    setStatus(data.msg + "  |  " + data.items.length + " item(s)", !data.ok);

    itemsDiv.innerHTML = "";

    if (data.items.length === 0) {
        itemsDiv.innerHTML = '<p class="empty">This folder is empty</p>';
        return;
    }

    for (const item of data.items) {
        const div = document.createElement("div");
        div.className = "item";

        const icon = item.type === "folder" ? "📁" : "📄";
        div.innerHTML = '<div class="icon">' + icon + '</div><div class="name"></div>';
        div.querySelector(".name").textContent = item.name;

        div.onclick = function () {
            document.querySelectorAll(".item").forEach(d => d.classList.remove("selected"));
            div.classList.add("selected");
        };

        if (item.type === "folder") {
            div.ondblclick = function () {
                callServer("/open", item.name);
            };
        }

        itemsDiv.appendChild(div);
    }
}

function setStatus(text, isError) {
    statusBar.textContent = text;
    if (isError) {
        statusBar.classList.add("error");
    } else {
        statusBar.classList.remove("error");
    }
}

document.getElementById("upBtn").onclick = function () {
    callServer("/up");
};

document.getElementById("newFolderBtn").onclick = function () {
    const name = prompt("Enter folder name:");
    if (name) {
        callServer("/mkdir", name.trim());
    }
};

document.getElementById("newFileBtn").onclick = function () {
    const name = prompt("Enter file name:");
    if (name) {
        callServer("/touch", name.trim());
    }
};

callServer("/list");
