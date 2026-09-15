import QtQuick 2.15

Rectangle {
    id: art
    radius: 12
    gradient: Gradient {
        GradientStop { position: 0; color: "#b4cd90" }
        GradientStop { position: 1; color: "#557b61" }
    }
    Canvas {
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            var p=getContext("2d")
            p.reset(); p.scale(width/180,height/180)
            p.strokeStyle="rgba(255,255,255,0.10)"; p.lineWidth=0.8
            for(var i=0;i<7;i++) { p.beginPath(); p.arc(10,174,70+i*16,-Math.PI/2,0); p.stroke() }
            var gradient=p.createLinearGradient(30,30,150,160)
            gradient.addColorStop(0,"#323a31"); gradient.addColorStop(0.4,"#111813"); gradient.addColorStop(0.65,"#343e34"); gradient.addColorStop(1,"#151d16")
            p.fillStyle=gradient; p.beginPath(); p.arc(90,90,65,0,Math.PI*2); p.fill()
            p.strokeStyle="rgba(196,217,188,0.13)"
            for(var r=32;r<60;r+=4) { p.beginPath(); p.arc(90,90,r,0,Math.PI*2); p.stroke() }
            p.fillStyle="#c5d7a5"; p.beginPath(); p.arc(90,90,25,0,Math.PI*2); p.fill()
            p.strokeStyle="#829967"; p.beginPath(); p.moveTo(72,84); p.lineTo(108,84); p.moveTo(72,96); p.lineTo(108,96); p.stroke()
            p.fillStyle="#202b1e"; p.beginPath(); p.arc(90,90,4,0,Math.PI*2); p.fill()
        }
    }
}
