var authentication = false;
var websocket_port = 0;
var websocket_IP = "";
var async_webcommunication = false;
var page_id = "";
var ws_source;
var log_off =false;
var websocket_started =false;
var esp_error_message ="";
var esp_error_code = 0;
var interval_ping = -1;
var last_ping = 0;
var enable_ping = false;
var spiffs_path = "";
var start_tab = "";
var FW_NAME = "ESP32 Project";
var FW_VERSION = 0.0
var ui_tabs = [];

function navbar(){
    var content="<table><tr>";
    var tlist = currentpath.split("/");
    var path="/";
    var nb = 1;
    content+="<td class='btnimg'  onclick=\"currentpath='/'; SendCommand('list','');\">/</td>";
    while (nb < (tlist.length-1))
        {
            path+=tlist[nb] + "/";
            content+="<td class='btnimg' onclick=\"currentpath='"+path+"'; SendCommand('list','');\">"+tlist[nb] +"</td><td>/</td>";
            nb++;
        }
        content+="</tr></table>";
    return content;
}

function padNumber(num, size) {
    var s = num.toString();
    while (s.length < size) s = "0" + s;
        return s;
}
function getPCTime(){
    var d = new Date();
    return d.getFullYear() + "-" + padNumber(d.getMonth() + 1 ,2) + "-" + padNumber(d.getDate(),2) + "-" + padNumber(d.getHours(),2) + "-" + padNumber(d.getMinutes(),2) + "-" + padNumber(d.getSeconds(),2); 
}

function HideAll(msg){
    //console.log("Hide all:" + msg);
    log_off = true;
    if(websocket_started){
        ws_source.close();
    }
    document.title = document.title + "(disconnected)";
    document.getElementById('MSG').innerHTML = msg;
    document.getElementById('settingstab').style.display = "none";
    document.getElementById('SPIFFStab.html').style.display = "none";
    document.getElementById('FWUPDATE').style.display = "none";
}

function FWError(){
    HideAll("Failed to communicate with FW!");
}

function FWOk() {
    //document.getElementById('MSG').innerHTML = "Connected";
    if (start_tab == "wizard") { wizardTab(); }
    else if (start_tab == "control") { controlTab(); }
    else { settingsTab(); }
    if (spiffs_path) { SendCommand('list',''); }
}

function InitUI(){
//removeIf(production)
var xmlhttp = {a:1};
//endRemoveIf(production)
if (typeof xmlhttp === 'undefined') {var xmlhttp = new XMLHttpRequest();}
//var xmlhttp = new XMLHttpRequest();
var url = "/command?commandText="+encodeURI("config.basics");
authentication = false;
async_webcommunication = false;
console.log("Init UI");
xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 ) { 
        console.log("XML HTTP Ready: " + xmlhttp.status, xmlhttp.responseText);
        var error = false;
        if(xmlhttp.status == 200) {
        var response = xmlhttp.responseText;
        var nbitem = 0;
        var tresp = response.split("#");
        if (tresp.length < 3) {
            error = true;
        } else {
            //FW name: ESP32 project # FW version: 0.2 # primary sd:/sd # secondary sd:No SD # authentication:no # webcommunication: Sync: /ws # hostname:esp-57CC # tabs: control,settings,files,wizard # start: settings
            for (var p=0; p < tresp.length; p++){
              var sublist = tresp[p].split(":");
              if (sublist[0].trim() == "FW name") {
                FW_NAME =  sublist[1];
                document.getElementById('FW_NAME').innerHTML = FW_NAME;
                nbitem++;
              }
              else if (sublist[0].trim() == "FW version"){
                 FW_VERSION =  sublist[1];
                 //document.getElementById('FW_VERSION').innerHTML = sublist[1];
                 nbitem++;
              }
              else if (sublist[0].trim() == "primary sd") {
                if (sublist[1].trim() == "No SD") {
                } else {
                    spiffs_path = sublist[1].trim();
                }
                nbitem++;
              }
              else if (sublist[0].trim() == "secondary sd") {
                nbitem++;
              }
              else if (sublist[0].trim() == "authentication"){
                 /*
                 if (sublist[1].trim() == "no") {
                     authentication = false;
                     document.getElementById('loginicon').style.visibility = "hidden";
                 }
                 else {
                     authentication = true;
                     document.getElementById('loginicon').style.visibility = "visible";
                 }
                 */
                 nbitem++;
               }
               else if (sublist[0].trim() == "webcommunication"){
                if (sublist[1].trim() == "Async") { async_webcommunication = true; }
                else {
                    async_webcommunication = false;
                    websocket_port = sublist[2].trim();
                    if (sublist.length>3) {
                        websocket_ip = sublist[3].trim();
                    } else {
                        console.log("No IP for websocket, use default");
                         websocket_ip =  document.location.hostname;
                    }
                    startSocket();
                }
                nbitem++;
              }
              else if (sublist[0].trim() == "hostname") {
                 document.title = sublist[1].trim();
                 nbitem++
              }
              else if (sublist[0].trim() == "tabs") {
                ui_tabs = sublist[1].trim().split(",");
                if (ui_tabs.includes("control")) { document.getElementById("control-button").style.display = "table-cell"; }
                if (ui_tabs.includes("settings")) { document.getElementById("settings-button").style.display = "table-cell"; }
                if (ui_tabs.includes("files") && spiffs_path) { document.getElementById("spiffs-button").style.display = "table-cell"; }
                if (ui_tabs.includes("wizard")) { document.getElementById("wizard-button").style.display = "table-cell"; }
                nbitem++
              }
              else if (sublist[0].trim() == "start") {
                start_tab = sublist[1].trim();
                nbitem++
              }
              else {
                console.log("Unknown basic config:", sublist[0].trim());
              }
           }
            if (nbitem == 9) {
               FWOk();
            } else {
                console.log("Invalid basic config items: " + nbitem);
                error = true;
            }
        }
            
        } else if (xmlhttp.status == 401){
            RL();
        } else {
            error = true;
            console.log( xmlhttp.status);
        }
        if (error) {
            FWError();
        }
    }
    };
    //removeIf(production)
    xmlhttp.readyState = 4;
    xmlhttp.status = 200;
    xmlhttp.responseText = "FW version: ROS Remote 0.2 # primary sd:/sd # secondary sd:No SD # authentication:no # webcommunication: Async # hostname:ROS-remote-57CC # tabs: control,settings,files,wizard # start: settings";
    xmlhttp.onreadystatechange();
    return;
    //endRemoveIf(production)
    xmlhttp.open("GET", url, true);
    xmlhttp.send();
}

var wsmsg = "";

function startSocket() {
    try {
        if (async_webcommunication) {
            ws_source = new WebSocket('ws://' + document.location.host + '/ws', ['arduino']);
        } else {
            console.log("Socket is " + websocket_ip + ":" + websocket_port);
            ws_source = new WebSocket('ws://' + websocket_ip + ':' + websocket_port, ['arduino']);
        }
    } catch (exception) {
        console.error(exception);
    }
    ws_source.binaryType = "arraybuffer";
    ws_source.onopen = function(e) {
        console.log("Connected");
    };
    ws_source.onclose = function(e) {
        console.log("Disconnected");
        //seems sometimes it disconnect so wait 3s and reconnect
        //if it is not a log off
        if (!log_off) setTimeout(startSocket, 3000);
    };
    ws_source.onerror = function(e) {
        //Monitor_output_Update("[#]Error "+ e.code +" " + e.reason + "\n");
        console.log("ws error", e);
    };
    ws_source.onmessage = function(e) {
        var msg = "";
        //bin
        if (e.data instanceof ArrayBuffer) {
            var bytes = new Uint8Array(e.data);
            for (var i = 0; i < bytes.length; i++) {
                msg += String.fromCharCode(bytes[i]);
                if ((bytes[i] == 10) || (bytes[i] == 13)) {
                    wsmsg += msg;
                    Monitor_output_Update(wsmsg);
                    process_socket_response(wsmsg);
                    //msg = wsmsg.replace("\n", "");
                    //wsmsg = msg.replace("\r", "");
                    console.log(wsmsg);
                    wsmsg = "";
                    msg = "";
                }
            }
            wsmsg += msg;
        } else if (e.data.startsWith("UPDATE:")) {
            process_state_update(e.data.substr(7));
        } else {
            msg += e.data;
            var tval = msg.split(":");
            if (tval.length >= 2) {
              if (tval[0] == 'CURRENT_ID') {
                    page_id = tval[1];
                    console.log("connection id = " + page_id);
                } else if (enable_ping && tval[0] == 'PING') {
                    page_id = tval[1];
                    console.log("ping from id = " + page_id);
                    last_ping = Date.now();
                    if (interval_ping == -1) interval_ping = setInterval(function() {
                        check_ping();
                    }, 10 * 1000);
                } else if (tval[0] == 'ACTIVE_ID') {
                    if (page_id != tval[1]) {
                        Disable_interface();
                    }
                } else if (tval[0] == 'ERROR') {
                  esp_error_message = tval[2];
                  esp_error_code = tval[1];
                  console.log("ERROR: " + tval[2] + " code:" +  tval[1]);
                  CancelCurrentUpload();
                } else if (tval[0] == 'MSG') {
                  console.log("MSG: " + tval[2] + " code:" +  tval[1]);
                }
                else {
                    console.log("Unknown WS message: " + msg);
                }
            }

        }
        //console.log(msg);

    };
}

function check_ping() {
    //if ((Date.now() - last_ping) > 20000){
    //Disable_interface(true);
    //console.log("No heart beat for more than 20s");
    //}
}

function ontogglePing(forcevalue) {
    if (typeof forcevalue != 'undefined') enable_ping = forcevalue;
    else enable_ping = !enable_ping;
    if (enable_ping) {
        if (interval_ping != -1) clearInterval(interval_ping);
        last_ping = Date.now();
        interval_ping = setInterval(function() {
            check_ping();
        }, 10 * 1000);
        console.log("enable ping");
    } else {
        if (interval_ping != -1) clearInterval(interval_ping);
        console.log("disable ping");
    }
}

window.onload = function() {
InitUI();
};

function uploadError()
{
    if (esp_error_code != 0) {
        alert('Update failed(' + esp_error_code + '): ' + esp_error_message);
        esp_error_code = 0;
    } else {
        alert('Update failed!');
    }
    
    if (typeupload == 1) {
        //location.reload();
        document.getElementById('upload-button').value = 'Upload';
        document.getElementById('prg').style.visibility = "hidden";
        document.getElementById('file-select').value="";
        SendCommand('list', '');
    } else {
        location.reload();
    }
}

function Uploadfile(){
if (!confirm("Confirm Firmware Update ?"))return;
var files = document.getElementById('fw-select').files;
if (files.length==0)return;
document.getElementById('ubut').style.visibility = 'hidden';
document.getElementById('fw-select').style.visibility = 'hidden';
document.getElementById('msg').style.visibility = "visible";
document.getElementById('msg').innerHTML="";
document.getElementById('SPIFFStab.html').style.display = "none";
document.getElementById('prgfw').style.visibility = "visible";
var formData = new FormData();
for (var i4 = 0; i4 < files.length; i4++) {
var file = files[i4];
var arg =  "/" + file.name + "S";
 //append file size first to check updload is complete
 formData.append(arg, file.size);
 formData.append('myfile[]', file, "/"+file.name);}
typeupload = 0;
xmlhttpupload = new XMLHttpRequest();
xmlhttpupload.open('POST', '/update', true);
//progress upload event
xmlhttpupload.addEventListener("progress", updateProgress, false);
//progress function
function updateProgress (oEvent) {
  if (oEvent.lengthComputable) {
    var percentComplete = (oEvent.loaded / oEvent.total)*100;
    document.getElementById('prgfw').value=percentComplete;
    // document.getElementById('msg').style.display = "block";
   document.getElementById('msg').innerHTML = "Uploading ..." + percentComplete.toFixed(0)+"%" ;
  } else {
    // Impossible because size is unknown
  }
}
xmlhttpupload.onload = function () {
 if (xmlhttpupload.status === 200) {
document.getElementById('ubut').value = 'Upload';
document.getElementById('MSG').style.display = "block";
document.getElementById('MSG').innerHTML="Restarting, please wait....";
document.getElementById('counter').style.visibility = "visible";
document.getElementById('ubut').style.visibility = 'hidden';
document.getElementById('ubut').style.width = '0px';
document.getElementById('fw-select').value="";
document.getElementById('fw-select').style.visibility = 'hidden';
document.getElementById('fw-select').style.width = '0px';

var jsonresponse = JSON.parse(xmlhttpupload.responseText);
if (jsonresponse.status=='1' || jsonresponse.status=='4' || jsonresponse.status=='1')uploadError();
if (jsonresponse.status=='2')alert('Update canceled!');
else if (jsonresponse.status=='3')
{
    var i5 = 0;
    var interval;
    var x = document.getElementById("prgfw");
    x.max=40;
    interval = setInterval(function(){
        i5=i5+1;
        var x = document.getElementById("prgfw");
        x.value=i5;
        document.getElementById('counter').innerHTML=41-i5;
        if (i5>40)
            {
            clearInterval(interval);
            location.reload();
            }
        },1000);
}
else uploadError()
 } else uploadError()
};
xmlhttpupload.send(formData);
}


function RL(){
    document.getElementById('loginpage').style.display='block';
}

function SLR (){
    document.getElementById('loginpage').style.display='none';
    var user = document.getElementById('lut').value.trim();
    var password = document.getElementById('lpt').value.trim();
    var url = "/login?USER="+encodeURIComponent(user) + "&PASSWORD=" + encodeURIComponent(password) + "&SUBMIT=yes" ;
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        if (xmlhttp.readyState == 4){
            if (xmlhttp.status != 200) {
                if (xmlhttp.status == 401) {
                    RL();
                } else {
                    FWError();
                    console.log(xmlhttp.status);
                }
            } else {
                InitUI();
            }
        }
    };
xmlhttp.open("GET", url, true);
xmlhttp.send();
}

// Create the modal
var listmodal = [];


function setactiveModal(html_template, closefunc) {
    if (typeof document.getElementById(html_template) === 'undefined') {
        console.log("Error: no " + html_template);
        return null;
    }
    var modal = new Object;
    modal.element = document.getElementById(html_template);
    modal.id = listmodal.length;
    modal.name = html_template;
    if (typeof closefunc !== 'undefined') modal.closefn = closefunc;
    else modal.closefn = myfnclose;
    listmodal.push(modal)
    //console.log("Creation of modal  " +  modal.name + " with ID " +modal.id);
    return listmodal[listmodal.length - 1];;
}

function getactiveModal() {
    if (listmodal.length > 0) {
        return listmodal[listmodal.length - 1];
    } else return null;
}

// open the modal 
function showModal() {
    var currentmodal = getactiveModal();
    currentmodal.element.style.display = "block";
    //console.log("Show modal " +  currentmodal.name + " with ID " + currentmodal.id  );
}

// When the user clicks on <span> (x), close the modal
function closeModal(response) {
    var currentmodal = getactiveModal();
    if (currentmodal != null) {
        currentmodal.element.style.display = "none";
        var closefn = currentmodal.closefn;
        //console.log("Deletetion of modal " +  currentmodal.name + " with ID "  + currentmodal.id);
        listmodal.pop();
        delete currentmodal;
        currentmodal = getactiveModal();
        //if (currentmodal != null)console.log("New active modal is  " +  currentmodal.name + " with ID "  + currentmodal.id);
        //else console.log("No active modal");
        closefn(response);
    }
}
//default close function
function myfnclose(value) {
    //console.log("modale closed: " + value);
}

function firmwareUpdateTab() {
    opentab('FWUPDATE', 'mainuitabscontent', 'mainuitablinks'); 
}
