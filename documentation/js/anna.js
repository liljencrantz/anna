
var anna = {
    init: function(){
	anna.makeToc();
	anna.syntaxHighlight();

	$(document).keypress(
	    function (evt)
	    {
		if(evt.keyCode == 27)
		{
		    $(".anna-code-popup").hide();
		}
	    }
	);
    },

    /*
      Create a toc based on the actual headers of the ain document
     */
    makeToc: function (){
	var toc = $(".toc");
	$(".anna-main").find("h1, h2").each(
	    function(idx, el)
	    {
		var text = el.textContent;
		
		var link = null;
		if(el.childNodes.length && el.childNodes[0].name)
		{
		    link = "#" + el.childNodes[0].name;
		}
		
		var res = $("<li>").addClass("anna-toc-"+el.nodeName.toLowerCase()).html($("<a>").text(text).attr("href", link));
		toc.append(res);
	    });
    },

    /*
      Very simple state-machine based syntax highlighter. Detexts
      string literals and comments and adds html markup for
      them. Nothing else.

      Will throw an exception on invalid code.
     */
    syntaxHighlight: function()
    {

	var pattern = [];

	$.each(["def", "return", "if", "else", "while", "as", "switch", "case", "cases", "default", "macro"], function (key, value) {
	    pattern.push({re: new RegExp("\\b(" + value + ")\\b", "g"), repl: "<span class='anna-keyword'>$1</span>"});
	});
	pattern.push({re: new RegExp("\\b([A-Z][a-z0-9A-Z_]*)\\b", "g"), repl: "<span class='anna-type'>$1</span>"});

	$(".anna-code").each(
	    function(idx, el)
	    {
		var html = "";
		var txt = el.textContent.replace("<", "&lt;").replace(">", "&gt;");
		var mode = "base";
		var comment_nest= 0;
		for(var i=0; i<txt.length; i++)
		{
		    var add = true;
		    switch(mode)
		    {
		    case "base":
			if(txt[i] == '"')
			{
			    mode = "string"
			    html += "<span class='anna-string-literal'>";
			}
			else if(txt[i] == '/')
			{
			    if(txt[i+1] == '/')
			    {
				mode = "comment";
				html += "<span class='anna-comment'>";
			    }
			    else if(txt[i+1] == '*')
			    {
				mode = "multi-comment";
				html += "<span class='anna-comment'>";
				comment_nest = 1;
			    }
			}
			break;

		    case "multi-comment":
			if((txt[i] == '/') &&(txt[i+1] == '*'))
			{
			    comment_nest++;
			}
			else if((txt[i] == '*') &&(txt[i+1] == '/'))
			{
			    comment_nest--;
			    if(comment_nest == 0)
			    {
				html += txt[i];
				html += txt[i+1];
				i++;
				mode = "base";
				html += "</span>";
				add = false;
			    }
			}
			break;
			
		    case "comment":
			if(txt[i] == '\n')
			{
			    mode = "base"
			    html += "</span>";
			}
			break;

		    case "string":
			if(txt[i] == '\\')
			{
			    html += txt[i];
			    i++;
			}
			else if(txt[i] == '"')
			{
			    mode = "base"
			    html += txt[i];
			    html += "</span>";
			    add = false;
			}
			break;
			
		    }
		    if(add)
		    {
			html += txt[i];
		    }
		}
		$.each(pattern, function (key, repl) {
		    html = html.replace(repl.re, repl.repl);
		});
		
		$(el).html(html);
	    }
	);
    }
};


$(document).ready(anna.init);
