const API = "http://localhost:8080";

const itemsDiv = document.getElementById("items");
const pathbar = document.getElementById("pathbar");
const statusBar = document.getElementById("status");
const menu = document.getElementById("menu");

// item that was right clicked
let menuItem = "";

async function callServer(route, name, newName) {
    let url = API + route;
    if (name) {
        url += "?name=" + encodeURIComponent(name);
    }
    if (newName) {
        url += "&newname=" + encodeURIComponent(newName);
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

        div.oncontextmenu = function (e) {
            e.preventDefault();
            div.click();
            menuItem = item.name;
            menu.style.left = e.clientX + "px";
            menu.style.top = e.clientY + "px";
            menu.style.display = "block";
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

document.getElementById("backBtn").onclick = function () {
    callServer("/back");
};

document.getElementById("forwardBtn").onclick = function () {
    callServer("/forward");
};

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

document.getElementById("renameOption").onclick = function () {
    const newName = prompt("Enter new name:", menuItem);
    if (newName) {
        callServer("/rename", menuItem, newName.trim());
    }
};

document.getElementById("deleteOption").onclick = function () {
    if (confirm("Delete " + menuItem + "? This cannot be undone.")) {
        callServer("/delete", menuItem);
    }
};

// hide the menu on any click, the option handlers above run first
document.addEventListener("click", function () {
    menu.style.display = "none";
});

callServer("/list");
