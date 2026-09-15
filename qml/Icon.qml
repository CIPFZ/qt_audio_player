import QtQuick 2.15

Canvas {
    id: icon
    property string name: "music"
    property color color: "#d5d9de"
    implicitWidth: 22
    implicitHeight: 22
    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        var p = getContext("2d")
        p.reset()
        p.scale(width / 24, height / 24)
        p.strokeStyle = color
        p.fillStyle = color
        p.lineWidth = 1.7
        p.lineCap = "round"
        p.lineJoin = "round"
        function line(x1,y1,x2,y2) { p.beginPath(); p.moveTo(x1,y1); p.lineTo(x2,y2); p.stroke() }
        function path(points,fill) {
            p.beginPath(); p.moveTo(points[0][0],points[0][1])
            for(var i=1;i<points.length;i++) p.lineTo(points[i][0],points[i][1])
            if(fill) { p.closePath(); p.fill() } else p.stroke()
        }
        if(name === "play") path([[9,5],[20,12],[9,19]],true)
        else if(name === "pause") { p.fillRect(7,5,3.5,14); p.fillRect(14,5,3.5,14) }
        else if(name === "next" || name === "previous") {
            if(name === "previous") { p.translate(24,0); p.scale(-1,1) }
            path([[6,6],[15,12],[6,18]],true); line(18,6,18,18)
        } else if(name === "add") { line(12,5,12,19); line(5,12,19,12) }
        else if(name === "folder") path([[3,7],[9,7],[11,9],[21,9],[21,19],[3,19],[3,7]],false)
        else if(name === "search") { p.beginPath(); p.arc(10,10,6,0,Math.PI*2); p.stroke(); line(15,15,20,20) }
        else if(name === "volume" || name === "muted") {
            path([[3,10],[7,10],[12,6],[12,18],[7,14],[3,14],[3,10]],false)
            if(name === "muted") { line(16,9,21,15); line(16,15,21,9) }
            else { p.beginPath(); p.arc(10,12,10,-0.7,0.7); p.stroke(); p.beginPath(); p.arc(10,12,6,-0.7,0.7); p.stroke() }
        } else if(name === "repeat" || name === "repeatOne") {
            path([[4,10],[4,7],[19,7],[16,4]],false); path([[20,14],[20,17],[5,17],[8,20]],false)
            if(name === "repeatOne") { p.font="8px sans-serif"; p.fillText("1",10,15) }
        } else if(name === "shuffle") {
            path([[4,6],[7,6],[17,18],[21,18],[18,15]],false)
            path([[4,18],[7,18],[17,6],[21,6],[18,3]],false)
        } else if(name === "sequential") {
            line(4,6,20,6); line(4,12,16,12); line(4,18,19,18); path([[17,15],[20,18],[17,21]],false)
        } else if(name === "trash") {
            line(5,7,19,7); line(9,4,15,4); path([[7,7],[8,20],[16,20],[17,7]],false); line(10,10,10,17); line(14,10,14,17)
        } else if(name === "close") { line(6,6,18,18); line(6,18,18,6) }
        else if(name === "locate") {
            p.beginPath(); p.arc(12,12,6,0,Math.PI*2); p.stroke(); line(12,2,12,6); line(12,18,12,22); line(2,12,6,12); line(18,12,22,12)
        } else {
            line(10,17,10,6); line(10,6,20,4); line(20,4,20,15)
            p.beginPath(); p.ellipse(4,15,6,4); p.stroke(); p.beginPath(); p.ellipse(14,13,6,4); p.stroke()
        }
    }
}
