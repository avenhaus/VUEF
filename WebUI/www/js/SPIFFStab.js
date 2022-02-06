//SPIFFS dialog
var currentpath = "/";
var xmlhttpupload;
var typeupload = 0;

function SPIFFStab(root) {
    opentab('SPIFFStab.html', 'mainuitabscontent', 'mainuitablinks'); 
    SendCommand('list','');
}

function select_dir(directoryname){
    currentpath+=directoryname + "/";
    SendCommand('list','');
}

function compareStrings(a, b) {
  // case-insensitive comparison
  a = a.toLowerCase();
  b = b.toLowerCase();
  return (a < b) ? -1 : (a > b) ? 1 : 0;
}

function dispatchfilestatus(jsonresponse)
{
var content ="";
content ="&nbsp;&nbsp;Status: "+jsonresponse.status;
content +="&nbsp;&nbsp;|&nbsp;&nbsp;Total: "+jsonresponse.total;
content +="&nbsp;&nbsp;|&nbsp;&nbsp;Used: "+jsonresponse.used;
content +="&nbsp;&nbsp;";
content +="<meter min='0' max='100' high='90' value='"+jsonresponse.occupation +"'></meter>&nbsp;"+jsonresponse.occupation +"%";
document.getElementById('status').innerHTML=content;
content ="";

if (currentpath!="/")
    {
     var pos = currentpath.lastIndexOf("/",currentpath.length-2);
     var previouspath = currentpath.slice(0,pos+1);
     content +="<tr style='cursor:hand;' onclick=\"currentpath='"+previouspath+"'; SendCommand('list','');\"><td >"+"<svg width='24' height='24' viewBox='0 0 24 24' version=\"2.0\"> <use href=\"#icon-back\" /> </svg>"+"</td><td colspan='4'> Up..</td><td></td><td></td></tr>";
    }
jsonresponse.files.sort(function(a, b) {
    return compareStrings(a.name, b.name);
});
var display_time =false;
for (var i1=0;i1 <jsonresponse.files.length;i1++){
//first display files
if (String(jsonresponse.files[i1].size) != "-1")
    {
    content +="<TR>";
    content +="<td><svg height='24' width='24' style='color:#5bc0de;' version=\"2.0\"> <use href=\"#icon-file\" /> </svg></td>";
    content +="<TD class='btnimg' style=\"padding:0px;\"><a href=\""+jsonresponse.path+jsonresponse.files[i1].name+"\" target=_blank><div class=\"blacklink\">";
    content +=jsonresponse.files[i1].name;
    content +="</div></a></TD><TD>";
    content +=jsonresponse.files[i1].size;
    content +="</TD>";
    if (jsonresponse.files[i1].hasOwnProperty('time')){
        display_time = true;
        content +="<TD>";
        content += jsonresponse.files[i1].time;
        content +="</TD>";
    } else {
    content +="<TD></TD>";    
    }
    content +="<TD width='0%'><div class=\"btnimg\" onclick=\"DeleteFile('"+jsonresponse.files[i1].name+"')\">";
    content +="<svg width='24' height='24' viewBox='0 0 128 128' version=\"2.0\" style='color:red;'> <use href=\"#icon-trash\" /> </svg></div></TD>";
    content +="<TD width='0%'><div class=\"btnimg\" onclick=\"RenameFile('"+jsonresponse.files[i1].name+"')\">";
    content +="<svg width='24' height='24' viewBox='0 0 128 128' version=\"2.0\" style='color:#888;'> <use href=\"#icon-edit\" /> </svg></div></TD>";
    //content +="<TD width='0%'><input class='btn btn-primary' type='button' style='padding: 3px 6px; background-color:#5bc0de;' onclick=\"Rename('"+jsonresponse.files[i1].name+"')\" value='Rename'/></TD>";
    content +="<td></td></TR>";
    }
}
//then display directories
for (var i2=0;i2 <jsonresponse.files.length;i2++){
if (String(jsonresponse.files[i2].size) == "-1")
    {
    content +="<TR><td><svg height='24' width='24' style='color:#5bc0de;' version=\"2.0\"> <use href=\"#icon-folder-open\" /> </svg></td>";
    content +="<TD  class='btnimg blacklink' style='padding:10px 15px;' onclick=\"select_dir('" + jsonresponse.files[i2].name+"');\">";
    content +=jsonresponse.files[i2].name;
    content +="</TD><TD></TD><TD></TD>";
    if (typeof jsonresponse.files[i2].hasOwnProperty('time')){
        display_time = true;
    }
    content +="<TD width='0%'><div class=\"btnimg\" onclick=\"DeleteDir('"+jsonresponse.files[i2].name+"')\">";
    content +="<svg width='24' height='24' viewBox='0 0 128 128' version=\"2.0\" style='color:red;'> <use href=\"#icon-trash\" /> </svg></div></TD>";
    content +="<TD width='0%'><div class=\"btnimg\" onclick=\"RenameDir('"+jsonresponse.files[i2].name+"')\">";
    content +="<svg width='24' height='24' viewBox='0 0 128 128' version=\"2.0\" style='color:#888;'> <use href=\"#icon-edit\" /> </svg></div></TD>";
    content +="<td></td></TR>";
    }
}
if(display_time){
    document.getElementById('FS_time').innerHTML = "";
} else {
    document.getElementById('FS_time').innerHTML = "Time";
}
 document.getElementById('file_list').innerHTML=content;
 document.getElementById('path').innerHTML=navbar();}

function DeleteFile(filename) {
    if (confirm("Confirm deletion of file: " + filename)) {
        SendCommand("delete",filename);
    }
}

function DeleteDir(filename) {
    if (confirm("Confirm deletion of directory: " + filename)) {
        SendCommand("delete_dir",filename);
    }
}

function CreateDir() {
var filename = prompt("Directory name", "");
if (filename != null) {
   SendCommand("create_dir",filename.trim());
   }
}

function RenameFile(currentName) {
    var newName = prompt("New file name", "");
    if (newName != null) {
       SendCommand("rename_file", currentName.trim(), newName.trim());
       }
}

function RenameDir(currentName) {
    var newName = prompt("New directory name", "");
    if (newName != null) {
       SendCommand("rename_dir", currentName.trim(), newName.trim());
       }
}


function SendCommand(action, filename) { SendCommand(action, filename, ""); }

function SendCommand(action, filename, newName) {
var xmlhttp = new XMLHttpRequest();
var url = "/files?action="+action;
//document.getElementById('MSG').style.display = "block";
//document.getElementById('MSG').innerHTML = "Connecting...";
url += "&path="+encodeURI(currentpath);
url += "&filename="+encodeURI(filename);
if (newName) {url += "&new-name="+encodeURI(newName);}
xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 ) {
        if(xmlhttp.status == 200) {
        var jsonresponse = JSON.parse(xmlhttp.responseText);
        dispatchfilestatus(jsonresponse);
        //document.getElementById('MSG').style.display = "none";
        //document.getElementById('MSG').innerHTML = "Connected";
    } else {
            if(xmlhttp.status == 401) { 
                RL ();
            } else {
                console.log(xmlhttp.status);
                FWError();
            }
        }
    }
};
xmlhttp.open("GET", url, true);
xmlhttp.send();
}

function Sendfile(){
var files = document.getElementById('file-select').files;
if (files.length==0)return;
document.getElementById('upload-button').value = "Uploading...";
document.getElementById('prg').style.visibility = "visible";
var formData = new FormData();
formData.append('path', currentpath);
for (var i3 = 0; i3 < files.length; i3++) {
var file = files[i3];
var arg = currentpath + file.name + "S";
 //append file size first to check updload is complete
 formData.append(arg, file.size);
 formData.append('myfiles[]', file, currentpath+file.name);}
xmlhttpupload = new XMLHttpRequest();
xmlhttpupload.open('POST', '/files', true);
//progress upload event
xmlhttpupload.upload.addEventListener("progress", updateProgress, false);
//progress function
function updateProgress (oEvent) {
  if (oEvent.lengthComputable) {
    var percentComplete = (oEvent.loaded / oEvent.total)*100;
    document.getElementById('prg').value=percentComplete;
    document.getElementById('upload-button').value = "Uploading ..." + percentComplete.toFixed(0)+"%" ;
  } else {
    // Impossible because size is unknown
  }
}
typeupload = 1;
xmlhttpupload.onload = function () {
 if (xmlhttpupload.status === 200) {
document.getElementById('upload-button').value = 'Upload';
document.getElementById('prg').style.visibility = "hidden";
document.getElementById('file-select').value="";
var jsonresponse = JSON.parse(xmlhttpupload.responseText);
dispatchfilestatus(jsonresponse);
 } else uploadError();
};

xmlhttpupload.send(formData);
}
