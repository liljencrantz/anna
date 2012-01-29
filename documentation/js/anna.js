/*
  Put all code in the anna namespace to avoid namespace pollution
*/
var anna = {

    currentPopup: null,
    init: function(){
	anna.makeToc();
	anna.syntaxHighlight();
	anna.initCodePopup();
	anna.initTables();
	anna.initInternal();
    },

    initInternal: function()
    {
	var toggle = $("#anna-internal-toggle");
	if(toggle.length > 0)
	{
	    var settings = {};
	    if('localStorage' in window)
	    {	
		settings = localStorage;
	    }
	    
	    var handler = function(){
		if(toggle[0].checked)
		{
		    $(".anna-internal").show();
		}
		else
		{
		    $(".anna-internal").hide();
		}
		settings.annaShowInternal = toggle[0].checked;
	    }

	    if('annaShowInternal' in settings)
	    {
		toggle[0].checked = settings.annaShowInternal == "true";
	    }

	    toggle.change(handler);
	    handler();
	}
    },

    initTables: function()
    {
	$(".anna-table tr:odd").addClass("anna-table-odd");
    },
    
    /*
      Handle code popups correctly
     */
    initCodePopup: function()
    {
	$(document).keypress(
	    function (evt)
	    {
		if(evt.keyCode == 27)
		{
		    $(".anna-code-popup").hide();
		    anna.currentPopup = null;
		}
	    }
	);
	$(".anna-code-popup-close").click(
	    function (evt)
	    {
		$(evt.target.parentNode.parentNode).hide();
		anna.currentPopup = null;
	    }
	);
    },

    /*
      Fade popup in or out.
    */
    togglePopup: function (selector)
    {
	var it = $(selector);
	if(anna.currentPopup)
	{
	    if(it[0] == anna.currentPopup)
	    {
		$(anna.currentPopup).fadeOut('fast');
		anna.currentPopup = null;
		return;
	    }
	    
	    $(anna.currentPopup).hide();
	}
	
	it.fadeIn('fast');
	anna.currentPopup = it[0];
    },
    
    /*
      Create a toc based on the actual headers of the main document
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
      Very simple syntax highlighter. Uses a state-machine to detect
      string literals and comments, and uses regexps to detect
      keywords, types and generic operators. 

      Very little error handling, anything might happen on invalid
      input.
    */
    syntaxHighlight: function()
    {

	var pattern = [];

	$.each(["type", "error", "enum", "var", "const", "def", "return", "if", "else", "while", "as", "switch", "case", "cases", "default", "macro", "or", "and"], function (key, value) {
	    pattern.push({regExp: new RegExp("\\b(" + value + ")\\b", "g"), replacement: "<span class='anna-keyword'>$1</span>"});
	});
	pattern.push({regExp: new RegExp("\\b(_*[A-Z][a-z0-9A-Z_]*)\\b", "g"), replacement: "<span class='anna-type'>$1</span>"});
	pattern.push({regExp: new RegExp("(\\^[a-z0-9A-Z_]*)\\b", "g"), replacement: "<span class='anna-keyword'>$1</span>"});

	$(".anna-code").each(
	    function(idx, el)
	    {
		var html = "";
		var txt = el.textContent.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
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
			else if(txt[i] == "'")
			{
			    mode = "char"
			    html += "<span class='anna-string-literal'>";
			}
			else if(txt[i] == '/')
			{
			    add = false;
			    mode = "base-slash";
			}
			break;

		    case "base-slash":
			if(txt[i] == '/')
			{
			    mode = "comment";
			    html += "<span class='anna-comment'>/";
			}
			else if(txt[i] == '*')
			{
			    mode = "multi-comment";
			    html += "<span class='anna-comment'>/";
			    comment_nest = 1;
			}
			else
			{
			    mode = "base";
			}
			break;

		    case "multi-comment":
			if(txt[i] == '/')
			{
			    mode = "multi-comment-slash";
			}
			else if(txt[i] == '*') 
			{
			    mode = "multi-comment-star";
			}
			break;

		    case "multi-comment-slash":
			if(txt[i] == '*')
			{
			    comment_nest++;
			}
			mode = "multi-comment";
			break;

		    case "multi-comment-star":
			mode = "multi-comment";
			if(txt[i] == '/')
			{
			    comment_nest--;
			    if(comment_nest == 0)
			    {
				html += "/";
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
			
		    case "char":
			if(txt[i] == '\\')
			{
			    html += txt[i];
			    i++;
			}
			else if(txt[i] == "'")
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
		    html = html.replace(repl.regExp, repl.replacement);
		});
		
		$(el).html(html);
	    }
	);
    }
};

$(document).ready(anna.init);
