/**
 * (c) Copyright 2013 - See the AUTHORS file for details.
 * (c) Ingmar Runge 2013
 *
 * Note: this only depends on jQuery and microevents.js, not angular.js!
 **/

function WebChatBackend(webChatSetup) {
	this.config = webChatSetup;
	this.baseUrl = webChatSetup.mod_path;
	this.websockets = false;
	this.backendState = { maxSeq: 1, backOffTime: 0, stopped: false, failingSince: 0 };
	this.windows = {}; // networkname.toLowerCase() => { statusWindow: StatusWindow, channelWindows: {
		// channame.toLowerCase() => ChannelWindow, ...}, queryWindows: { nick.toLowerCase() => QueryWindow, ...} }
};

(function(global, $) { // private scope

	var
		// source: https://github.com/BYK/Modernizr/commit/3fc9f8ef585d3bf47db74925541ba4b8ff10a216
		WebSocketsSupported = ('WebSocket' in global && global.WebSocket.CLOSING === 2);

	// public methods are exposed through this:
	WebChatBackend.prototype = {

		loadUserData: function(onSuccess, onFail) {
			var me = this,
			jqxhr = $.getJSON(this.baseUrl + 'user_data',
				function(data) {
					var nws = data.users[0].networks;
					// *TODO* move sorting to a place where it can be invoked if something changes as well
					nws.sort(function(n1, n2) {
						return n1.name.localeCompare(n2.name);
					});
					for(var nk in nws) {
						var network = nws[nk];
						network.channels.sort(function(c1, c2) {
							return c1.name.localeCompare(c2.name);
						});
						var winEntry = me.windows[network.name.toLowerCase()] = {
							statusWindow: new StatusWindow(network),
							channelWindows: {},
							queryWindows: {}
						};
						for(var ck in network.channels) {
							var chan = network.channels[ck];
							winEntry.channelWindows[chan.name.toLowerCase()] = new ChannelWindow(network.name, chan);
						}
					}
					onSuccess(data);
				});

			if(onFail) {
				// *TODO* error handling, retry etc.
				jqxhr.fail(onFail)
			}
		},

		stop: function() {
			this.backendState.stopped = true;
			// *TODO* close websocket
		},

		liveConnect: function() {
			if(WebSocketsSupported) {
				/*/ if(!web socket connect blah) // *TODO*
				// fallback 
				// else this.websockets = true;*/
			}

			if(!this.websockets) {
				cometBackendCore.call(this);
			}
		},

		sendChatMessage: function(targetType, targetNetwork, targetName, isAction, msgText, onFail) {
			var messageData = { target_type: targetType, target_network: targetNetwork, target_name: targetName,
					text: msgText, msg_is_action: isAction, action: 'send_chat_message' };

			if(this.websockets) {
				// *TODO* send message via websockets
			} else {
				// *TODO* encapsulate POST logic
				messageData[this.config.csrf_token_name] = this.config.csrf_token_value;

				var self = this,
					jqxhr = $.post(this.baseUrl + 'user_action', messageData,
					function(data) {
						if(!data && onFail) {
							onFail();
						}
					}, 'json');

				jqxhr.fail(function() {
					// *TODO* retry mechanism
					if(onFail) {
						onFail();
					}
					self.trigger('backend_connection_problem');
				});
			}
		},

		getWindowList: function() {
			return this.windows;
		},

		getWindowInstance: function(type, network, name) {
			network = network.toLowerCase();
			if(this.windows[network]) {
				if(type == 'status')
					return this.windows[network].statusWindow;
				else if(type == 'chan')
					return (this.windows[network].channelWindows[name.toLowerCase()] || null);
				else if(type == 'query')
					return (this.windows[network].queryWindows[name.toLowerCase()] || null);
			}
			return null;
		}

	};

	/* these are private methods: */

	function logProblem(msg) {
		if(console && console.log) {
			console.log(msg);
		}
		return null;
	}

	function processLiveBackendData(event) {
		// event's structure = { type: string, timestamp: unix_time, payload: {} }
		// - the exact payload depends on type.

		var self = this, windowType, windowKey, window = null,
			pl = event.payload;

		/* generic handler that populates the window variable for a channel window */
		function findChanWindow() {
			window = self.getWindowInstance('chan', pl.network, pl.chan.name);

			if(!window) {
				return logProblem('Warning: could not map event "' + event.type + '" to channel "' + pl.chan.name + '".');
			}

			return true;
		}

		/* generic handler that populates the window variable for a network / status window */
		function findNetworkWindow() {
			window = self.getWindowInstance('status', pl.network);

			if(!window) {
				return logProblem('Warning: could not map event "' + event.type + '" to network "' + pl.network + '".');
			}

			return true;
		}

		/* generic handler that populates the window variable for a query window */
		function createQueryWindow() {
			window = self.getWindowInstance('query', pl.network, pl.person.nick);

			if(!window) {
				if(!self.windows[pl.network.toLowerCase()]) {
					return logProblem('Unknown network for new query window: "' + pl.network + '".');
				} else {
					self.windows[pl.network.toLowerCase()].queryWindows[pl.person.nick.toLowerCase()] = new QueryWindow(pl.person);

					self.trigger('query_window_created', window);
				}
			}

			return true;
		}

		/* generic handler that adds an event to the window's buffer */
		function bufferEvent() {
			if(!window) {
				return logProblem('Trying to buffer event "' + event.type + '" without a defined window.');
			}

			window.addToBuffer(event);

			return true;
		}

		/* generic handler that passes a backend event through to subscribers */
		function triggerEventSimple() {
			self.trigger(event.type, event);
			return true;
		}

		/* handler that adds a person to windows' nick list */
		function addPersonToChanWindow() {
			window.addUser(event.payload.person);
			return true;
		}

		/* handler that removes a person from windows' nick list */
		function removePersonFromChanWindow() {
			window.removeUser(event.payload.person);
			return true;
		}

		/* specific handler for the chan_quit event */
		function chanQuitHandler() {
			for(var k in pl.channels) {
				var window = self.getWindowInstance('chan', pl.network, pl.channels[k].name);
				// split event per channel:
				if(window) {
					var new_event = { type: event.type, payload: { network: pl.network, chan: pl.channels[k],
						person: pl.person, quit_msg: pl.quit_msg } };
					window.addToBuffer(new_event);
					window.removeUser(pl.person);
					self.trigger(event.type, new_event);
				}
			}
			return true;
		}

		/* specific handler for the user_join event */
		function createChanWindow() {
			var isNewWindow = false;

			window = self.getWindowInstance('chan', pl.network, pl.chan.name);

			if(!window) {
				if(!self.windows[pl.network.toLowerCase()]) {
					return logProblem('Unknown network for new channel window: "' + pl.network + '".');
				} else {
					self.windows[pl.network.toLowerCase()].channelWindows[pl.chan.name.toLowerCase()] =
						window = new ChannelWindow(pl.network, pl.chan);
					isNewWindow = true;
				}
			}

			window.joined = true;

			if(isNewWindow) {
				// user has just joined the channel and the channel hasn't been in our list before.
				self.trigger('chan_window_created', window);
			} else {
				// update "joined" status in frontend (currently mimicking a chan_update event, this is not clean):
				self.trigger('chan_update', event);
			}

			return true;
		}

		/* specific handler for the user_part event */
		function userPartHandler() {
			var net = self.windows[pl.network.toLowerCase()];

			if(net) {
				delete net.channelWindows[pl.chan.name.toLowerCase()];
			}

			return true;
		}

		/* specific handler for the user_join_notif event */
		function userJoinNotifHandler() {
			window.joined = true;
			// update "joined" status in frontend (currently mimicking a chan_update event, this is not clean):
			self.trigger('chan_update', event);
			return true;
		}

		/* specific handler for the chan_update event */
		function chanUpdateHandler() {
			// there's pl.updated: "string" if we need it.
			var chan = event.payload.chan;
			if(chan.nicklist) window.batchNickListUpdate(chan.nicklist);
			return true;
		}

		/* specific handler for the irc_conn event */
		function ircConnHandler() {
			if(window.connected && !event.payload.connected) {
				// *TODO* clear channel nick lists and stuff
			}
			window.connected = event.payload.connected;
			return true;
		}

		var handlers = {
			msg_chan: [ findChanWindow, bufferEvent, triggerEventSimple ],
			msg_query: [ createQueryWindow, bufferEvent, triggerEventSimple ],
			chan_join: [ findChanWindow, bufferEvent, addPersonToChanWindow, triggerEventSimple ],
			chan_part: [ findChanWindow, bufferEvent, removePersonFromChanWindow, triggerEventSimple ],
			chan_quit: [ chanQuitHandler ],
			user_join: [ createChanWindow ],
			user_join_notif: [ findChanWindow, userJoinNotifHandler ],
			user_part: [ userPartHandler, triggerEventSimple ],
			chan_update: [ findChanWindow, chanUpdateHandler, triggerEventSimple ],
			irc_conn: [ findNetworkWindow, ircConnHandler, triggerEventSimple ],
			//znc_broadcast: [ findNetworkWindow, , triggerEventSimple ],
		};

		var handled = false;
		if(handlers[event.type]) {
			handled = true;
			for(var k in handlers[event.type]) {
				if(!handlers[event.type][k]()) {
					logProblem('Handler [' + k + '] for event "' + event.type + '" failed.');
					handled = false;
					break;
				}
			}
		} else {
			logProblem('There\'s no handler for event "' + event.type + '".');
		}

		return handled;
	}

	function cometBackendCore() {
		var self = this;
		$.ajax({
			dataType: 'json',
			url: this.baseUrl + 'comet?seq=' + this.backendState.maxSeq,
			cache: false,
			timeout: 55 * 1000,
			success: function(data) {
				self.backendState.failingSince = 0;

				if(!data) {
					self.trigger('backend_connection_problem'); // this can stop() us
				} else if(!cometBackendResponse.call(self, data)) {
					++self.backendState.backOffTime;
				} else if(self.backendState.backOffTime > 0) {
					--self.backendState.backOffTime;
				}

				// *TODO* some logic to back off a little if too many messages are flooding in.

				cometBackendNext.call(self);
			},
			error: function(jqXHR, textStatus, errorThrown) {
				if(textStatus != 'timeout') {
					++self.backendState.backOffTime; // error seems worse than the 55/60 second COMET timeout

					if(jqXHR.status >= 400) {
						// session is probably gone or invalid
						self.trigger('backend_connection_problem');
					} else if(self.backendState.failingSince == 0) {
					// using this to get a 120 second timeout for loss of network connectivity etc.:
						self.backendState.failingSince = Date.now();
					} else if(Date.now() - self.backendState.failingSince > 120 * 1000) {
						self.trigger('backend_connection_problem');
					}
				}

				cometBackendNext.call(self);
			}
		});
	}

	function cometBackendResponse(seqs) {
		var oldMaxSeq = this.backendState.maxSeq;

		for(var i = 0; i < seqs.length; i++) {
			var seq = seqs[i].seq;

			if(seq + 1 > this.backendState.maxSeq) {
				if(i == 0 && seq != this.backendState.maxSeq && this.backendState.maxSeq > 1) {
					// we have missed a message. roll over and play dead for now.
					self.trigger('backend_connection_problem');
				} else {
					this.backendState.maxSeq = seq + 1;
				}
			} else if(seq < this.backendState.maxSeq) {
				/* The webchat module will always use higher numbers unless ZNC
					is restarted (in which case the session will become invalid and the user
					will have to log in again, but they may do so in a different tab/window),
					or the module is being unloaded and loaded again.
					In any case, this could turn out pretty bad. */

				self.trigger('backend_connection_problem');

				return false;
			}

			processLiveBackendData.call(this, seqs[i].event);
		}

		return (this.backendState.maxSeq > oldMaxSeq);
	}

	function cometBackendNext() {
		if(!this.backendState.stopped) {
			var self = this;
			setTimeout(function() { cometBackendCore.call(self); }, this.backendState.backOffTime * 1000);
		}
	}

	function websocketBackend() {
		// *TODO* websocket backend...
	}

	MicroEvent.mixin(WebChatBackend);

	/* window base class definitions and tools */

	var BaseWindowPrototype = {
		addToBuffer: function(event) {
			this.buffer.push(event);
		},
		replayBuffer: function(handler) {
			$.each(this.buffer, function(idx, event) {
				handler(event);
			});
		},
		// getFlagsString(): must be defined by child classes
	};

	var BaseWindowGetters = {
		flagsString: function() { return this.getFlagsString(); }
	};

	function ExtendWindowClass(proto, extensions) {
		$.extend(proto, BaseWindowPrototype, extensions);
		$.each(BaseWindowGetters, function(name, func) {
			Object.defineProperty(proto, name, { get: func, enumerable: true });
		});
	}

	/* utility functions */

	function boolFlagStr(obj, name) {
		return (obj[name] ? name + ' ' : '');
	}

	/* status window class */

	function StatusWindow(network) {
		this.name = network.name;
		this.buffer = [];
		this.connected = network.connected;
		this.configuredNick = network.configured_nick;
	}

	ExtendWindowClass(StatusWindow.prototype, {
		getFlagsString: function() {
			return boolFlagStr(this, 'connected');
		},
	});

	/* channel window class */

	function ChannelWindow(network, chan) {
		this.network = network;
		this.name = chan.name;
		this.buffer = [];
		this.topic = chan.topic || '';
		this.nicklist = chan.nicklist || [];
		this.joined = chan.joined;
		this.enabled = chan.enabled;
		this.detached = chan.detached;

		this.sortNicklist();
	}

	ExtendWindowClass(ChannelWindow.prototype, {
		getFlagsString: function() {
			return boolFlagStr(this, 'joined');
		},
		sortNicklist: function() {
			var roles = { regular: 0, voice: 1, halfop: 3, op: 5 };
			this.nicklist.sort(function(a, b) {
				return (roles[b.role] - roles[a.role]) || a.nick.localeCompare(b.nick);
			});
		},
		getNicklist: function() {
			return this.nicklist;
		},
		_findNicklistIndex: function(nick) {	
			nick = nick.toLowerCase();
			for(var k in this.nicklist) {
				if(this.nicklist[k].nick.toLowerCase() === nick) {
					return k;
				}
			}
			return -1;
		},
		addUser: function(person) {
			var idx = this._findNicklistIndex(person.nick);
			if(idx == -1) {
				this.nicklist.push(person);
				this.sortNicklist();
			}
		},
		removeUser: function(person) {
			var idx = this._findNicklistIndex(person.nick);
			if(idx > -1) {
				this.nicklist.splice(idx, 1);
			}
		},
		batchNickListUpdate: function(newList) {
			this.nicklist = newList;
			this.sortNicklist();
		},
	});

	/* query window class */

	function QueryWindow(person) {
		this.name = person.nick;
		this.buffer = [];
		this.person = person;
	}

	ExtendWindowClass(QueryWindow.prototype, {
		getFlagsString: function() {
			return '';
		},
	});

})(window, jQuery);
