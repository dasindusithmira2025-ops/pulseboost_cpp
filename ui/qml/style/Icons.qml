pragma Singleton
import QtQuick 2.15

QtObject {
    function glyph(name) {
        if (name === "dashboard") return "\u25A6"
        if (name === "charts") return "\u223F"
        if (name === "game") return "\u25B6"
        if (name === "ram") return "\u25C8"
        if (name === "temps") return "\u25CE"
        if (name === "drivers") return "\u2692"
        if (name === "processes") return "\u2699"
        if (name === "storage") return "\u25A4"
        if (name === "network") return "\u25C9"
        if (name === "startup") return "\u25B7"
        if (name === "history") return "\u25F4"
        if (name === "security") return "\u26E8"
        if (name === "settings") return "\u2630"
        if (name === "chat") return "\u25EC"
        if (name === "bolt") return "\u26A1"
        if (name === "sparkles") return "\u2726"
        if (name === "chart") return "\u223F"
        if (name === "file") return "\u25A4"
        if (name === "restore") return "\u21BA"
        if (name === "activity") return "\u25CE"
        if (name === "rocket") return "\u25B7"
        if (name === "wifi") return "\u25C9"
        if (name === "shield") return "\u26E8"
        return "\u25CF"
    }
}
