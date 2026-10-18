const API = "http://localhost:8080";

const itemsDiv = document.getElementById("items");
const pathbar = document.getElementById("pathbar");
const statusBar = document.getElementById("status");
const menu = document.getElementById("menu");
const treeDiv = document.getElementById("tree");

// item that was right clicked
let menuItem = "";

// paths of the folders that are open in the tree view
const expanded = { "root": true };

async function callServer(route, name, newName, path) {
    let url = API + route + "?x=1";
    if (name) {
        url += "&name=" + encodeURIComponent(name);
    }
    if (newName) {
        url += "&newname=" + encodeURIComponent(newName);
    }
    if (path) {
        url += "&path=" + encodeURIComponent(path);
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

    // open all the parents of the current folder in the tree
    const parts = data.path.split("/");
    let p = "";
    for (let i = 0; i < parts.length; i++) {
        p = i === 0 ? parts[0] : p + "/" + parts[i];
        expanded[p] = true;
    }
    treeDiv.innerHTML = "";
    drawTree(data.tree, data.tree.name, data.path, treeDiv);

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
            e.stopPropagation();
            div.click();
            menuItem = item.name;
            showMenu(e, true);
        };

        if (item.type === "folder") {
            div.ondblclick = function () {
                callServer("/open", item.name);
            };
        }

        itemsDiv.appendChild(div);
    }
}

// draws one folder of the tree and then its sub folders
function drawTree(node, path, currentPath, parentDiv) {
    const row = document.createElement("div");
    row.className = "treerow";
    if (path === currentPath) {
        row.classList.add("current");
    }

    const hasChildren = node.children.length > 0;
    const isOpen = expanded[path] === true;

    const arrow = document.createElement("span");
    arrow.className = "arrow";
    if (hasChildren) {
        arrow.textContent = isOpen ? "▼" : "▶";
    }
    row.appendChild(arrow);

    const label = document.createElement("span");
    label.textContent = "📁 " + node.name;
    row.appendChild(label);

    // arrow only opens/closes, clicking the name goes into the folder
    arrow.onclick = function (e) {
        e.stopPropagation();
        expanded[path] = !isOpen;
        callServer("/list");
    };
    row.onclick = function () {
        callServer("/goto", null, null, path);
    };
    parentDiv.appendChild(row);

    if (hasChildren && isOpen) {
        const childDiv = document.createElement("div");
        childDiv.className = "treechildren";
        for (const child of node.children) {
            drawTree(child, path + "/" + child.name, currentPath, childDiv);
        }
        parentDiv.appendChild(childDiv);
    }
}

// onItem is true when an item was right clicked, false for empty space
function showMenu(e, onItem) {
    document.getElementById("newFolderOption").style.display = onItem ? "none" : "block";
    document.getElementById("newFileOption").style.display = onItem ? "none" : "block";
    document.getElementById("renameOption").style.display = onItem ? "block" : "none";
    document.getElementById("deleteOption").style.display = onItem ? "block" : "none";

    menu.style.left = e.clientX + "px";
    menu.style.top = e.clientY + "px";
    menu.style.display = "block";
}

function setStatus(text, isError) {
    statusBar.textContent = text;
    if (isError) {
        statusBar.classList.add("error");
    } else {
        statusBar.classList.remove("error");
    }
}

function newFolder() {
    const name = prompt("Enter folder name:");
    if (name) {
        callServer("/mkdir", name.trim());
    }
}

function newFile() {
    const name = prompt("Enter file name:");
    if (name) {
        callServer("/touch", name.trim());
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

document.getElementById("newFolderBtn").onclick = newFolder;
document.getElementById("newFileBtn").onclick = newFile;
document.getElementById("newFolderOption").onclick = newFolder;
document.getElementById("newFileOption").onclick = newFile;

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

// right click on empty space of the folder
itemsDiv.oncontextmenu = function (e) {
    e.preventDefault();
    document.querySelectorAll(".item").forEach(d => d.classList.remove("selected"));
    showMenu(e, false);
};

// hide the menu on any click, the option handlers above run first
document.addEventListener("click", function () {
    menu.style.display = "none";
});

callServer("/list");
