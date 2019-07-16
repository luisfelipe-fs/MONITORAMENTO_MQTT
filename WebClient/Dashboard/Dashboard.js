var topics = ["CONTROL/ECP-LAB1", "CONTROL/ECP-LAB2"];
var enabled = true;

random = Math.floor(Math.random() * 1000).toString();
client = new Paho.MQTT.Client("iot.eclipse.org", 80, "ECP-MASTER" + random);

client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

client.connect({onSuccess:onConnect});

function publish (topic, msg) {
    message = new Paho.MQTT.Message(msg);
    message.destinationName = topic;
    client.send(message);
}

function onConnect() {
  $("#tr1-light").bootstrapToggle("enable");
  $("#tr1-air").bootstrapToggle("enable");  
  $("#tr1-automatic").bootstrapToggle("enable");
  publish(topics[0], "1GP");
  publish(topics[0], "1GL");
  publish(topics[0], "1GA");
  publish(topics[0], "1GS");
  console.log("Connected.");
  client.subscribe("MASTER/DATA");
  //client.subscribe(topics[0]);
}

function onConnectionLost(responseObject) {
  console.log("Disconnected.");
  client.connect({onSuccess:onConnect});
}

function onMessageArrived(message) {
  msg = message.payloadString;
  console.log("TOPIC:"+message.destinationName+"|"+"MSG:"+msg);
  if (msg.slice(0, 3) == "1GP") {
      if (msg[3] == "0")
        $("#tr1-pir").text("Não");
      else if (msg[3] == "1")
        $("#tr1-pir").text("Sim");
  }
  if (msg.slice(0, 3) == "1GL") {
      $("#tr1-light").off("change");
      if (msg[3] == "0") {
        if (enabled)
            $("#tr1-light").bootstrapToggle("on");
        else {
            $("#tr1-light").bootstrapToggle("enable");
            $("#tr1-light").bootstrapToggle("on");
            $("#tr1-light").bootstrapToggle("disable");    
        }
      }
      else if (msg[3] == "1") {
        if (enabled)
            $("#tr1-light").bootstrapToggle("off");
        else {
            $("#tr1-light").bootstrapToggle("enable");
            $("#tr1-light").bootstrapToggle("off");
            $("#tr1-light").bootstrapToggle("disable");
        }
      }
      onToggle("#tr1-light", "1SL0", "1SL1");
  }
  if (msg.slice(0, 3) == "1GA") {
      $("#tr1-air").off("change");
      if (msg[3] == "0") {
        if (enabled)
            $("#tr1-air").bootstrapToggle("on");
        else {
            $("#tr1-air").bootstrapToggle("enable");
            $("#tr1-air").bootstrapToggle("on");
            $("#tr1-air").bootstrapToggle("disable");
        }
      }
      else if (msg[3] == "1") {
        if (enabled)
            $("#tr1-air").bootstrapToggle("off");
        else {
            $("#tr1-air").bootstrapToggle("enable");
            $("#tr1-air").bootstrapToggle("off");
            $("#tr1-air").bootstrapToggle("disable");
        }
      }
      onToggle("#tr1-air", "1SA0", "1SA1");
  }
  if (msg.slice(0, 3) == "1GS") {
      $("#tr1-automatic").off("change");
      if (msg[3] == "0") {
        $("#tr1-automatic").bootstrapToggle("on");
        $("#tr1-light").bootstrapToggle("enable");
        $("#tr1-air").bootstrapToggle("enable");
        enabled = true;
      }
      else if (msg[3] == "1") {
        $("#tr1-automatic").bootstrapToggle("off");
        $("#tr1-light").bootstrapToggle("disable");
        $("#tr1-air").bootstrapToggle("disable");
        enabled = false;
      }
      onToggleAuto("#tr1-automatic", "1SS0", "1SS1");
  }
}

function onToggle (id, tmsg, fmsg) {
    $(id).change(function () {
        if ($(id).prop("checked"))
            publish(topics[0], tmsg);
        else
            publish(topics[0], fmsg);
    })
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
    })
}