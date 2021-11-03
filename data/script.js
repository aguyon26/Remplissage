$(document).ready(function(){
    $("#appliquer").click(function(){
        var valeur = $("#choixDelayLed").val();
        $.post("delayLed",{
            valeurDelayLed: valeur
        });
    });
});
// Valeur par défauts 
$(document).ready(function(){
    $.ajax( {
        type: "GET",
        url: "param.xml",
        dataType: "xml",
        success: function(xml) 
                 {
                   $(xml).find("param").each(
                     function()
                     {
                        $(this).find("valeurSaisi").each(
                        function()
                        {   var id = $(this).attr("id");
                            document.getElementById("valeurSaisi"+id).value = $(this).text();
                            $(this).value =document.getElementById("valeurSaisi"+id).value+1;
                      });
                    });

                }
    });
    
    $.ajax({
        url: "wparam.xml",
        data: "<test>1</test>", 
        type: "POST",
        contentType: "text/xml",
        dataType: "text",
        success : parse,
        error : function (xhr, ajaxOptions, thrownError){  
            console.log(xhr.status);          
            console.log(thrownError);
        } 
    }); 
});

// A trvailler faire une boucle d'action... (1..4)
$(document).ready(function(){
    $("#appliquerParametre").click(function(){
        var valeur = $("#valeurSaisi1").val();
        $.post("valeurSaisi1",{
            valeurSaisi1: valeur
        });
    });
    $("#appliquerParametre").click(function(){
        var valeur = $("#valeurSaisi2").val();
        $.post("valeurSaisi2",{
        valeurSaisi2: valeur
        });                
    });
    $("#appliquerParametre").click(function(){
        var valeur = $("#valeurSaisi3").val();
        $.post("valeurSaisi3",{
            valeurSaisi3: valeur
        });
    });
    $("#appliquerParametre").click(function(){
        var valeur = $("#valeurSaisi4").val();
        $.post("valeurSaisi4",{
            valeurSaisi4: valeur
        });
    });
    $("#appliquerParametre").click(function(){
        var valeur = XMLString();
        $.post("wparamxml",{
            wparamxml: valeur
        });
    });
});

// Solution a optimsier (lire le XML et changer la valeur de la balise pour le renvoyer dans le fichier...)
function XMLString ()
{
var file ="<?xml version='1.0' encoding='UTF-8' standalone='yes'?>";
file += "<param>";
file += "<valeurSaisi id='1'>"+document.getElementById("valeurSaisi1").value+"</valeurSaisi>";
file += "<valeurSaisi id='2'>"+document.getElementById("valeurSaisi2").value+"</valeurSaisi>";
file += "<valeurSaisi id='3'>"+document.getElementById("valeurSaisi3").value+"</valeurSaisi>";
file += "<valeurSaisi id='4'>"+document.getElementById("valeurSaisi4").value+"</valeurSaisi>";
file += "</param>";
return file;
}

// A trvailler pour détecter qui répond 1 et 2 
setInterval(function getData() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("lectureValeur1").innerHTML = this.responseText;
        }
        
    };

    xhttp.open("GET","lectureValeur1", true);
    xhttp.send();
}, 2000);


setInterval(function getData() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("lectureValeur2").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET","lectureValeur2", true);
    xhttp.send();
}, 2000);


function onButton() {
    var xhttp = new XMLHttpRequest();
    document.getElementById("ON").innerText = "ON..";
    document.getElementById("OFF").innerText = "OFF";

    xhttp.open("GET", "on", true);
    xhttp.send();
}

function offButton() {
    var xhttp = new XMLHttpRequest();
    document.getElementById("ON").innerText = "ON";
    document.getElementById("OFF").innerText = "OFF..";
    xhttp.open("GET", "off", true);
    xhttp.send();
}
