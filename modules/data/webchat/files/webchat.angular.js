/**
 * (c) Copyright 2013 - See the AUTHORS file for details.
 * (c) Ingmar Runge 2013
 **/

if(!window.WebChatSetup) {
	throw 'WebChatSetup is not defined.';
}

(function(global, angular, $) {
	'use strict';

	/* helper methods */

	function _angular_async_this(f_name) {
		var f_args = Array.prototype.slice.call(arguments, 1),
			f_this = this; /* ... yeah ... */
		return this.scope.$apply(function() { return f_this[f_name].apply(f_this, f_args); });
	}

	(function(global) {
		var replacements = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' };
		global.escape_xml = function(str) {
			str = new String(!str && str !== 0 ? '' : str);
			return str.replace(/[&<>"]/g, function(ch) { return replacements[ch]; });
		};
	})(global);

	/* set up angular module */

	var WebChat = angular.module('WebChat', []), // capitalizing because it's sort of a namespace.
		zncBackend = new WebChatBackend(global.WebChatSetup),
		navigationController,
		windowMgrController;

	/* title bar controller */

	WebChat.controller('TitleBar', ['$scope', function($scope) {
		$scope.user_name = global.WebChatSetup.user_name;
	}]);

	/* navigation bar controller */

	function WebChat_NavigationController(scope) {
		this.scope = scope;
	};

	WebChat_NavigationController.prototype = {
		callIn: _angular_async_this,

		fullUpdate: function() {
			this.scope.windows = zncBackend.getWindowList();
		},

		windowListChanged: function() {
			// doesn't need to do anything at this point, $apply and
			// the reference this.scope.windows will take care of things.
		},

		_findLinkType: function(target) {
			var parent = target.parent(),
				TYPES = [ 'status', 'chan', 'query' ];
			for(var k in TYPES) {
				if(parent.hasClass(TYPES[k])) {
					return TYPES[k];
				}
			}
		},

		onMsgChan: function(event) {
		
		},
	};

	WebChat.controller('Navigation', ['$scope', function($scope) {
		// things exposed to template:
		$scope.windows = [];

		$scope.switchWindow = function(event) {
			var target = $(event.target),
				targetName = target.text(),
				targetType = navigationController._findLinkType(target),
				targetNetwork = target.parents('ul.windows').data('network');

			windowMgrController.showWindow(targetType, targetNetwork, targetName);
		};

		// this class contains methods that are not exposed to the template:
		navigationController = new WebChat_NavigationController($scope);
	}]);

	/* window manager controller */

	function WebChat_WindowManagerController(scope) {
		this.scope = scope;
		this.idCounter = 0;
		this.activeWindowId = null;
	};

	WebChat_WindowManagerController.prototype = {
		callIn: _angular_async_this,

		reset: function() {
			this.scope.domWindows = {};
		},

		showWindow: function(type, network, name) {
			var winKey = type + '/' + network.toLowerCase() + '/' + name.toLowerCase(),
				domWindow;

			for(var k in this.scope.domWindows) {
				this.scope.domWindows[k].visible = false;
			}

			if(this.scope.domWindows[winKey]) {
				domWindow = this.scope.domWindows[winKey];
				domWindow.visible = true;
			} else {
				domWindow = this.scope.domWindows[winKey] = {
						type: type,
						name: name,
						network: network,
						id: ++this.idCounter,
						visible: true,
						lines: [],
						// *very bad i18n properties.*
						displayName: (type == 'status' ? 'Status window' : name),
						hasNicklist: (type == 'chan'),
					};

				var backendWindow = zncBackend.getWindowInstance(type, network, name),
					mgr = this;

				if(!backendWindow) {
					// this means that there is a bug somewhere :(
					return;
				}

				backendWindow.replayBuffer(function(event) {
					mgr.genericBackendEventSink(domWindow, event);
				});

				this.syncWindowStatus(backendWindow, domWindow);

				this.scrollToBottom(domWindow, true);
			}

			// *TODO* remove inactive windows from DOM
			// if the total number exceeds N and there is no
			// input in the windows' <textarea>s.

			this.activeWindowId = domWindow.id;
		},

		removeWindow: function(type, network, name) {
			var winKey = type + '/' + network.toLowerCase() + '/' + name.toLowerCase();

			delete this.scope.domWindows[winKey];
		},

		syncWindowStatus: function(backendWindow, domWindow) {
			var type = domWindow.type;

			if(type == 'status') {
				domWindow.disconnected = !backendWindow.connected;
			} else {
				var backendStatusWindow = zncBackend.getWindowInstance('status', domWindow.network);

				if(backendStatusWindow) {
					domWindow.disconnected = !backendStatusWindow.connected;

					if(type == 'chan') {
						domWindow.notJoined = backendStatusWindow.connected && !backendWindow.joined;
					}
				}
			}

			if(type == 'chan') {
				domWindow.nicklist = backendWindow.getNicklist();
			}
		},

		scrollToBottom: function(domWindow, force) {
			// *TODO* store if user has changed scroll pos and adhere to "force" argument.

			// the "timeout" ensures that angular.js updates the DOM before we scroll:
			setTimeout(function() {
				var div = $('#window-' + domWindow.id + ' .lines');
				div.animate({ scrollTop: div.prop('scrollHeight') }, 500);
			});
		},

		genericBackendEventSink: function(domWindow, event) {
			var map = {
				msg_chan: 'lineFromMessageEvent',
				msg_query: 'lineFromMessageEvent',
				chan_join: 'lineFromJoinPartEvent',
				chan_part: 'lineFromJoinPartEvent',
				chan_quit: 'lineFromQuitEvent',
			};

			if(map[event.type]) {
				var line = this[map[event.type]](event);

				if(event.timestamp) {
					// uses browser timezone. supporting the ZNC user's timezone would be pretty hard.
					var tm = moment.utc(event.timestamp + ' +0000', 'ddd MMM DD HH:mm:ss YYYY Z');
					line.time = tm.local().format('HH:mm:ss');
				}

				if(line) {
					if(line.body) {
						line.bodyHtml = escape_xml(line.body);
						delete line.body;
					}
					domWindow.lines.push(line);
				}
			}

			return null;
		},

		findDomWindowForChannelEvent: function(event) {
			var pl = event.payload,
				winKey = 'chan/' + pl.network.toLowerCase() + '/' + pl.chan.name.toLowerCase();

			if(!this.scope.domWindows[winKey]) {
				// window not currently in DOM.
				return null;
			}

			return this.scope.domWindows[winKey];
		},

		backendChannelEventHandler: function(event) {
			var domWindow = this.findDomWindowForChannelEvent(event);
			if(!domWindow) {
				return;
			}

			if(event.type == 'chan_update') {
				var pl = event.payload,
					backendWindow = zncBackend.getWindowInstance('chan', pl.network, pl.chan.name);

				if(!backendWindow) {
					return;
				}

				this.syncWindowStatus(backendWindow, domWindow);
			} else {
				this.genericBackendEventSink(domWindow, event);

				if(domWindow.id == this.activeWindowId) {
					this.scrollToBottom(domWindow, false);
				}
			}
		},

		// when the IRC connection is lost or established again:
		ircConnChange: function(event) {
			// this ain't a beauty.
			for(var k in this.scope.domWindows) {
				var domWindow = this.scope.domWindows[k];
				if(domWindow.network.toLowerCase() == event.payload.network.toLowerCase()) {
					var backendWindow = zncBackend.getWindowInstance(domWindow.type, domWindow.network, domWindow.name);
					if(backendWindow) {
						this.syncWindowStatus(backendWindow, domWindow);
					}
				}
			}
		},

		lineFromMessageEvent: function(event) {
			var line = {
				nick: event.payload.person.nick,
			};

			var body = global.escape_xml(event.payload.msg_body);

			// *TODO* linkify etc. message contents here

			line.bodyHtml = body;

			return line;
		},

		lineFromJoinPartEvent: function(event) {
			return {
				// *TODO* make this suck less
				body: event.payload.person.nick +
					(event.type == 'chan_part' ? ' left' : ' joined') + ' the channel'
			};
		},

		lineFromQuitEvent: function(event) {
			return {
				body: event.payload.person.nick + ' quit from the network'
			}
		},

		findDomWindowById: function(windowId) {
			for(var k in this.scope.domWindows) {
				if(this.scope.domWindows[k].id == windowId) {
					return this.scope.domWindows[k];
				}
			}
			return null;
		},

		userAction: function(windowId, inputText, onFail) {
			var domWindow = this.findDomWindowById(windowId),
				messageIsAction = false;

			if(!domWindow || !inputText)
				return false;

			if(inputText.substr(0, 1) == '/') {
				if(inputText.substr(0, 4) == '/me ') {
					messageIsAction = true;
					inputText = inputText.substr(4).trim();
				} else {
					alert('Not implemented yet, sorry about that!'); // *TODO*
					return false;
				}
			}

			if(domWindow.type == 'status') {
				alert('You can\'t send messages to a status window.');
				return false;
			}

			zncBackend.sendChatMessage(domWindow.type, domWindow.network, domWindow.name,
				messageIsAction, inputText, onFail);

			return true;
		},

		nickCompletion: function(windowId, prefix) {
			var domWindow = this.findDomWindowById(windowId),
				matches = [];
			prefix = prefix.toLowerCase();

			if(!domWindow.nicklist) {
				if(domWindow.name.substr(0, prefix.length).toLowerCase() == prefix) {
					return [ domWindow.name ];
				}
				return null;
			}

			for(var k in domWindow.nicklist) {
				var nick = domWindow.nicklist[k].nick;
				if(nick.substr(0, prefix.length).toLowerCase() == prefix) {
					matches.push(nick);
				}
			}

			return matches;
		},

		findWindowIdFromHtmlElement: function(element) {
			var
				htmlElement = $(element),
				htmlWindow = htmlElement.parents('div.window'),
				windowId = parseInt(htmlWindow.attr('id').replace(/^window-/, ''));
			return windowId;
		},
	};

	WebChat.controller('WindowManager', ['$scope', function($scope) {
		$scope.domWindows = {};

		$scope.closeWindow = function() {
		
		};

		windowMgrController = new WebChat_WindowManagerController($scope);
	}]);

	/* main startup/initialization routine */

	function initWebChat() {
		initTextAreaKeyBindings();

		zncBackend.loadUserData(function(data) {
			$('#wc-loading').hide();
			navigationController.callIn('fullUpdate');
			$('#wc-main').show();

			zncBackend.liveConnect({});
		});

		// *TODO* use angular.js routes!
	};

	/* handle <textarea> events via jQuery, could/should probably be implemented
	  as an angular.js "directive". */

	function initTextAreaKeyBindings() {
		$('#wc-windows').on('keypress', 'textarea',
			function(event) {
				if(event.which != 13) {
					return;
				}
				event.preventDefault();

				var textarea = $(event.target),
					windowId = windowMgrController.findWindowIdFromHtmlElement(textarea),
					inputText = textarea.val().trim(),
					failCallback = function() {
						textarea.val(inputText);
						alert('Sending the message failed.');
					};

				if(windowMgrController.userAction(windowId, inputText, failCallback)) {
					textarea.val('');
				}
			});

		var lastWord = '',
			lastMatch = '',
			lastMatchIndex = 0,
			shiftDown = false;

		$('#wc-windows').on('keyup keydown', function(event) {
				shiftDown = event.shiftKey;
			});

		$('#wc-windows').on('keydown', 'textarea',
			function(event) {
				if(event.which != 9) {
					if(event.which >= 45) // something that does more than modifying the caret pos
						lastWord = '';
					return;
				}
				var textarea = $(event.target),
					caretPos = event.target.selectionStart,
					text = textarea.val(),
					word = text.substr(0, caretPos).match(/[^\s:]+?$/),
					wordStartPos, windowId,
					windowId = windowMgrController.findWindowIdFromHtmlElement(textarea),
					nextMatchIndex;
				if(!word && lastMatch) {
					if(text.substr(0, lastMatch.length + 2) == lastMatch + ': ') {
						word = lastWord;
						wordStartPos = 0;
					}
				} else if(word[0] == lastMatch) {
					wordStartPos = caretPos - lastMatch.length;
					word = lastWord;
				} else if(word) {
					word = word[0];
					wordStartPos = caretPos - word.length;
				}
				if(word && windowId) {
					// known problem: joins + parts that occur between two tab key presses and affect the list
					// are not handled, so nicks may show up two times or not at all in that case.
					var matches = windowMgrController.nickCompletion(windowId, word),
						nextDir = (shiftDown ? -1 : 1);
					if(lastWord !== word || lastMatchIndex + nextDir >= matches.length) {
						nextMatchIndex = 0;
					} else if(lastMatchIndex + nextDir < 0) {
						nextMatchIndex = matches.length - 1;
					} else {
						nextMatchIndex = lastMatchIndex + nextDir;
					}
					lastWord = word;
					if(matches && matches[nextMatchIndex]) {
						word = lastMatch = matches[nextMatchIndex];
						if(wordStartPos == 0) word += ': ';
						textarea.val(text.substr(0, wordStartPos) + word + text.substr(caretPos));
						event.target.selectionEnd = event.target.selectionStart = wordStartPos + word.length;
					}
					lastMatchIndex = nextMatchIndex;
				}
				return false; // do not process this event anywhere else.
			});
	}

	/* backend event bindings */

	zncBackend.bind('msg_chan', function(event) {
		navigationController.callIn('onMsgChan', event);
		windowMgrController.callIn('backendChannelEventHandler', event);
	});

	var bceh = function(event) {
		windowMgrController.callIn('backendChannelEventHandler', event);
	};
	zncBackend.bind('chan_join', bceh);
	zncBackend.bind('chan_part', bceh);
	zncBackend.bind('chan_quit', bceh);
	zncBackend.bind('chan_update', bceh);

	zncBackend.bind('irc_conn', function(event) {
		windowMgrController.callIn('ircConnChange', event);
	});

	zncBackend.bind('chan_window_created', function(backendWindow) {
		navigationController.callIn('windowListChanged');
		// no need to update the window manager, it will pull the new window when the user clicks it.
	});

	zncBackend.bind('user_part', function(event) {
		navigationController.callIn('windowListChanged');
		windowMgrController.callIn('removeWindow', 'chan', event.payload.network, event.payload.chan.name);
	});

	zncBackend.bind('backend_connection_problem', function() {
		zncBackend.stop();
		$('#backend-connection-gone').show();
	});

	// init when DOM is ready:
	$(initWebChat);

})(window, angular, jQuery);
