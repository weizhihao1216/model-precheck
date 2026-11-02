import QtQuick 2.11

Item {
    id: root
    property string name: "check"
    property color color: "#0d9488"
    property real size: 18

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    onColorChanged: canvas.requestPaint()
    onNameChanged: canvas.requestPaint()
    onSizeChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: Canvas.Image

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            var w = width;
            var h = height;
            if (w <= 0 || h <= 0) return;

            ctx.strokeStyle = root.color;
            ctx.fillStyle = root.color;
            ctx.lineWidth = Math.max(1.5, w * 0.09);
            ctx.lineCap = "round";
            ctx.lineJoin = "round";

            if (root.name === "home") {
                ctx.beginPath();
                ctx.moveTo(w * 0.5, h * 0.15);
                ctx.lineTo(w * 0.15, h * 0.48);
                ctx.lineTo(w * 0.26, h * 0.48);
                ctx.lineTo(w * 0.26, h * 0.85);
                ctx.lineTo(w * 0.44, h * 0.85);
                ctx.lineTo(w * 0.44, h * 0.62);
                ctx.lineTo(w * 0.56, h * 0.62);
                ctx.lineTo(w * 0.56, h * 0.85);
                ctx.lineTo(w * 0.74, h * 0.85);
                ctx.lineTo(w * 0.74, h * 0.48);
                ctx.lineTo(w * 0.85, h * 0.48);
                ctx.closePath();
                ctx.stroke();
            } else if (root.name === "code") {
                ctx.beginPath();
                ctx.moveTo(w * 0.30, h * 0.30);
                ctx.lineTo(w * 0.12, h * 0.50);
                ctx.lineTo(w * 0.30, h * 0.70);
                ctx.moveTo(w * 0.62, h * 0.22);
                ctx.lineTo(w * 0.38, h * 0.78);
                ctx.moveTo(w * 0.70, h * 0.30);
                ctx.lineTo(w * 0.88, h * 0.50);
                ctx.lineTo(w * 0.70, h * 0.70);
                ctx.stroke();
            } else if (root.name === "flash" || root.name === "special") {
                ctx.beginPath();
                ctx.moveTo(w * 0.55, h * 0.12);
                ctx.lineTo(w * 0.22, h * 0.52);
                ctx.lineTo(w * 0.48, h * 0.52);
                ctx.lineTo(w * 0.42, h * 0.88);
                ctx.lineTo(w * 0.78, h * 0.46);
                ctx.lineTo(w * 0.52, h * 0.46);
                ctx.closePath();
                ctx.stroke();
            } else if (root.name === "check") {
                ctx.beginPath();
                ctx.moveTo(w * 0.20, h * 0.52);
                ctx.lineTo(w * 0.42, h * 0.75);
                ctx.lineTo(w * 0.82, h * 0.25);
                ctx.stroke();
            } else if (root.name === "report") {
                ctx.beginPath();
                ctx.moveTo(w * 0.20, h * 0.15);
                ctx.lineTo(w * 0.58, h * 0.15);
                ctx.lineTo(w * 0.80, h * 0.37);
                ctx.lineTo(w * 0.80, h * 0.85);
                ctx.lineTo(w * 0.20, h * 0.85);
                ctx.closePath();
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(w * 0.58, h * 0.15);
                ctx.lineTo(w * 0.58, h * 0.37);
                ctx.lineTo(w * 0.80, h * 0.37);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(w * 0.32, h * 0.52);
                ctx.lineTo(w * 0.68, h * 0.52);
                ctx.moveTo(w * 0.32, h * 0.68);
                ctx.lineTo(w * 0.68, h * 0.68);
                ctx.stroke();
            } else if (root.name === "gear" || root.name === "settings") {
                var cx = w * 0.5;
                var cy = h * 0.5;
                var rIn = w * 0.26;
                var rOut = w * 0.40;
                ctx.beginPath();
                for (var j = 0; j < 8; j++) {
                    var ang = j * Math.PI / 4;
                    var b1 = ang - 0.18;
                    var b2 = ang - 0.10;
                    var b3 = ang + 0.10;
                    var b4 = ang + 0.18;
                    if (j === 0) ctx.moveTo(cx + rIn * Math.cos(b1), cy + rIn * Math.sin(b1));
                    else ctx.lineTo(cx + rIn * Math.cos(b1), cy + rIn * Math.sin(b1));
                    ctx.lineTo(cx + rOut * Math.cos(b2), cy + rOut * Math.sin(b2));
                    ctx.lineTo(cx + rOut * Math.cos(b3), cy + rOut * Math.sin(b3));
                    ctx.lineTo(cx + rIn * Math.cos(b4), cy + rIn * Math.sin(b4));
                }
                ctx.closePath();
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(cx, cy, w * 0.14, 0, Math.PI * 2);
                ctx.stroke();
            } else if (root.name === "alert" || root.name === "warning") {
                ctx.beginPath();
                ctx.moveTo(w * 0.5, h * 0.20);
                ctx.lineTo(w * 0.5, h * 0.60);
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(w * 0.5, h * 0.78, Math.max(1.2, w * 0.06), 0, Math.PI * 2);
                ctx.fill();
            } else if (root.name === "search") {
                var scx = w * 0.42;
                var scy = h * 0.42;
                var sr = w * 0.26;
                ctx.beginPath();
                ctx.arc(scx, scy, sr, 0, Math.PI * 2);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(scx + sr * 0.707, scy + sr * 0.707);
                ctx.lineTo(w * 0.82, h * 0.82);
                ctx.stroke();
            }
        }
    }
}
