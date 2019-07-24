var DATA_TOPIC = "MASTER/DATA";
var DATA_REGEX = /[ALPU][GS][01]/g;
var topics = ["CONTROL/ECP-LAB1", "CONTROL/ECP-LAB2"];
var enabled = true;

random = Math.floor(Math.random()*1000).toString();
client = new Paho.MQTT.Client("broker.hivemq.com", 8000, "ECP-MASTER"+random);

client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

client.connect({onSuccess:onConnect});

function publish (topic, msg) {
    message = new Paho.MQTT.Message(msg);
    message.destinationName = topic;
    client.send(message);
}

function enable_toggle () {
    $("#tr1-light").bootstrapToggle("enable");
    $("#tr1-air").bootstrapToggle("enable");  
    $("#tr1-automatic").bootstrapToggle("enable");
}

function onConnect () {
    enable_toggle();
    publish(topics[0], "1PGLGAGUG");
    console.log("Connected.");
    client.subscribe(DATA_TOPIC);
}

function onConnectionLost(responseObject) {
    console.log("Disconnected. Reconnecting...");
    client.connect({onSuccess:onConnect});
}

function onToggle (id, tmsg, fmsg) {
    $(id).change(function () {
        if ($(id).prop("checked"))
            publish(topics[0], tmsg);
        else
            publish(topics[0], fmsg);
    });
}

function onToggleAuto (id, tmsg, fmsg) {
    $(id).change(function () {
        if ($(id).prop("checked")) {
            publish(topics[0], tmsg);
            $("#tr1-light").bootstrapToggle("enable");
            $("#tr1-air").bootstrapToggle("enable");
            enabled = true;
        }
        else {
            publish(topics[0], fmsg);
            $("#tr1-light").bootstrapToggle("disable");
            $("#tr1-air").bootstrapToggle("disable");
            enabled = false;
        }
    });
}

function discreteToggle (opt, id, onMsg, offMsg) {
    $(id).off("change");
    if (opt == "0") {
        if (enabled) {
            $(id).bootstrapToggle("on");
        }
        else {
            $(id).bootstrapToggle("enable");
            $(id).bootstrapToggle("on");
            $(id).bootstrapToggle("disable");    
        }
    }
    else if (opt == "1") {
        if (enabled) {
            $(id).bootstrapToggle("off");
        }
        else {
            $(id).bootstrapToggle("enable");
            $(id).bootstrapToggle("off");
            $(id).bootstrapToggle("disable");
        }
    }
    onToggle(id, onMsg, offMsg);
}

function onMessageArrived (message) {
    payload = message.payloadString;
    console.log("TOPIC:"+message.destinationName+"|"+"MSG:"+payload);
    commandList = payload.match(DATA_REGEX);
    for (command of commandList) {
        if (command.slice(0, 2) == "LG") {
            discreteToggle(command[2], "#tr1-light", "1LS0", "1LS1");
        }
        if (command.slice(0, 2) == "AG") {
            discreteToggle(command[2], "#tr1-air", "1AS0", "1AS1");
        }
        if (command.slice(0, 2) == "PG") {
            if (command[2] == "0") {
                $("#tr1-pir").text("Não");
            }
            else if (command[2] == "1") {
                $("#tr1-pir").text("Sim");
            }
        }
        if (command.slice(0, 2) == "UG") {
            $("#tr1-automatic").off("change");
            if (command[2] == "0") {
                $("#tr1-automatic").bootstrapToggle("on");
                $("#tr1-light").bootstrapToggle("enable");
                $("#tr1-air").bootstrapToggle("enable");
                enabled = true;
            }
            else if (command[2] == "1") {
                $("#tr1-automatic").bootstrapToggle("off");
                $("#tr1-light").bootstrapToggle("disable");
                $("#tr1-air").bootstrapToggle("disable");
                enabled = false;
            }
            onToggleAuto("#tr1-automatic", "1US0", "1US1");
        }
    }
}