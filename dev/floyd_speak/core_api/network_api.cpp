//
//  network_api.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "network_api.h"




//////////////////////////////////////		URLS


//	???
std::string escape_string_to_url_string(const std::string& s){
	return s;
}

//	???
std::string unescape_string_from_url_string(const std::string& s){
	return s;
}

//	???
bool is_valid_url(const std::string& s){
	return false;
}

//	???
url_t make_url(const std::string& s){
	assert(false);
	return {};
}


//	???
url_parts_t split_url(const url_t& url){
	assert(false);
	return {};
}

//	???
url_t make_urls(const url_parts_t& parts){
	assert(false);
	return {};
}



#if 0

	/*
			"http://api.bossbattle.xyz/" => "api.bossbattle.xyz"
		*/
	public static string GetURLBody(string url){
		string result = url.Trim();
		if(result.StartsWith("http://")){
			result = result.Remove (0, 7);
		}
		if(result.EndsWith("/")){
			result = result.Remove (result.Length - 1, 1);
		}
		return result;
	}


	[TestAttribute]
	static private void Prove_RESTManager_GetURLBody__PrefixAndSuffix(){
		string result = GetURLBody("http://api.bossbattle.xyz/");
		quark.UTVerify(result == "api.bossbattle.xyz");
	}
	[TestAttribute]
	static private void Prove_RESTManager_GetURLBody__PRefix(){
		string result = GetURLBody("http://api.bossbattle.xyz");
		quark.UTVerify(result == "api.bossbattle.xyz");
	}
	[TestAttribute]
	static private void Prove_RESTManager_GetURLBody__Suffix(){
		string result = GetURLBody("api.bossbattle.xyz/");
		quark.UTVerify(result == "api.bossbattle.xyz");
	}

	[TestAttribute]
	static private void Prove_RESTManager_GetURLBody__WhiteSpace(){
		string result = GetURLBody("    http://api.bossbattle.xyz/      ");
		quark.UTVerify(result == "api.bossbattle.xyz");
	}


	public static bool IsValidURL(string url){
		return IsValidBaseURL(url);
	}
#endif












#if 0
//////////////////////////////////////		REST





	public enum Status {
		kOK = 1000,
		kTimeout,
		kMissingResponseDefect,	//	??? This is not a problem now that we use Parse.com, right?
		kNotFound,
		kUnknownError,
		kBadRequest
	}

	

	public enum Type {
		kGET,
		kPOST,
		kPUT
	};
	
	public class RequestParams {
		public RequestParams(){
			_headers = new Dictionary<string, string>();
			_params = new Dictionary<string, string>();
			quark.Assert(CheckInvariant());
		}
		
		public RequestParams(RequestParams original){
			quark.Assert(original.CheckInvariant());
			
			_headers = new Dictionary<string, string>(original._headers);
			_params = new Dictionary<string, string>(original._params);
			_rawData = original._rawData;
			
			quark.Assert(CheckInvariant());
		}
		
		public bool CheckInvariant(){
			foreach(var h in _headers){
				quark.Assert(h.Key.Length > 0);
				quark.Assert(h.Value.Length > 0);
			}
			
			foreach(var h in _params){
				quark.Assert(h.Key.Length > 0);
				quark.Assert(h.Value.Length > 0);
			}
			return true;
		}
		
		
		////////////		State
		public Dictionary<string, string> _headers;
		public Dictionary<string, string> _params;
		public byte[] _rawData = null;
	}
	

	//		GETResult

	public class GETResult {
		public GETResult(Status status, string response, string debugInfo, Exception bestHTTPException){
			quark.Assert(response != null);
			quark.Assert(debugInfo != null);

			_status = status;
			_response = response;
			_debugInfo = debugInfo;
			_bestHTTPException = bestHTTPException;
		}
		
		public bool CheckInvariant(){
			quark.Assert(this != null);
			return true;
		}
		
		public string Status {
			get { return _status.ToString(); }
		}
		public Status _status;
		public string _response;
		public string _debugInfo;
		public readonly Exception _bestHTTPException;
	};
	

	public class Support {
		public static System.Exception MakeException(GETResult result){
			string message = "[REST error] " + result._debugInfo + " [" + result._status.ToString() + "] ";
			string message2 = result._debugInfo + " [" + result._status.ToString() + "] ";

			if(result._status == Status.kOK){
				quark.Assert(false);
				return null;
			}
			else if(result._status == Status.kTimeout){
				return quark.MakeTimeoutException(URLHelpers.kRESTTargetType, message2, result._bestHTTPException);
			}
			else if(result._status == Status.kMissingResponseDefect){
				return quark.MakeNotFoundException(URLHelpers.kRESTTargetType, message2, "");
			}
			else if(result._status == Status.kNotFound){
				return quark.MakeNotFoundException(URLHelpers.kRESTTargetType, message2, "");
			}
			else if(result._status == Status.kUnknownError){
				return new System.ArgumentException(message, "");
			}
			else if(result._status == Status.kBadRequest){
				return quark.MakeArgumentException(URLHelpers.kRESTTargetType, message2, "");
			}
			else{
				return new System.ArgumentException(message, "");
			}
		}

		//	### response.IsFromCache?
		//	Returns all HTML errors as ints, not by throwing exceptions.
		public static GETResult InterpretResponse(HTTPRequest request){
			quark.Assert (request != null);

			string debugInfo = "##debug info: [" + request.State.ToString() + "]";

			var bestHTTPException = request.Exception;

			if(request.State == HTTPRequestStates.Finished){
				quark.Assert (request.Response != null);
				
				HTTPResponse response = request.Response;
				int htmlStatusCode = response.StatusCode;
				string responseText = response.DataAsText;

				/*
					2015-01-28: we have problem with getting 200 "OK" and an empty response string. This is a temporary
					kludge306.
				*/

				//	HTML - OK
				if(htmlStatusCode == 200){
					if(responseText == ""){
						return new GETResult(Status.kMissingResponseDefect, "", debugInfo, bestHTTPException);
					}
					else{
						return new GETResult(Status.kOK, responseText, debugInfo, bestHTTPException);
					}
				}

				//	HTML - Not Modified
				if(htmlStatusCode == 304){
					if(responseText == ""){
						return new GETResult(Status.kMissingResponseDefect, "", debugInfo, bestHTTPException);
					}
					else{
						return new GETResult(Status.kOK, responseText, debugInfo, bestHTTPException);
					}
				}
				
				//	HTML - Created
				if(htmlStatusCode == 201){
					return new GETResult(Status.kOK, responseText, debugInfo, bestHTTPException);
				}
				
				//	HTML - Bad Request
				else if(htmlStatusCode == 400){
					return new GETResult(Status.kBadRequest, "", debugInfo, bestHTTPException);
				}
				
				//	HTML - Not Found
				else if(htmlStatusCode == 404){
					return new GETResult(Status.kNotFound, "", debugInfo, bestHTTPException);
				}
				else{
					return new GETResult(Status.kUnknownError, "", debugInfo, bestHTTPException);
				}
			}
			else if(request.State == HTTPRequestStates.Error){
				string s = bestHTTPException.ToString ();
				return new GETResult(Status.kUnknownError, "", s, bestHTTPException);
			}
			else if(request.State == HTTPRequestStates.Aborted){
				return new GETResult(Status.kUnknownError, "", debugInfo, bestHTTPException);
			}
			else if(request.State == HTTPRequestStates.ConnectionTimedOut){
				return new GETResult(Status.kTimeout, "", debugInfo, bestHTTPException);
			}
			else if(request.State == HTTPRequestStates.TimedOut){
				return new GETResult(Status.kTimeout, "", debugInfo, bestHTTPException);
			}
			else{
				return new GETResult(Status.kUnknownError, "", debugInfo, bestHTTPException);
			}
		}
		
		public static HTTPMethods TypeToBestHTTPType(Type type){
			if(type == Type.kGET){
				return HTTPMethods.Get;
			}
			else if(type == Type.kPOST){
				return HTTPMethods.Post;
			}
			else if(type == Type.kPUT){
				return HTTPMethods.Put;
			}
			else{
				quark.Assert(false);
				return HTTPMethods.Get;
			}
			/*
			public enum HTTPMethods
				{
					Get,
					Head,
					Post,
					Put,
					Delete,
					Patch
				}
			*/
		}
	}




	////////////////////////////////////		RESTManager

		public static float kConnectTimeoutSeconds = 20;

	/*
		Use an instance ofthis object to make REST requests and get data back.
		Normally only one instance of this object is needed.
	*/

	public class RESTManager {
		public static int kYieldMS = 300;
		//		public static float kConnectTimeoutSeconds = 20000;
		
		

		/*
			Make a REST-request to the URL. Returns a future-object holding the result.
			This function is async - it does not wait for server reply but returns at once.
			If you want to block, call GetSync() on the returned future or poll it.
		*/
		public Future<string> SendRequest(Type type, string url, RequestParams p = null){
			quark.Assert(CheckInvariant ());
			quark.Assert (URLHelpers.IsValidURL(url));
			quark.Assert (p == null || p.CheckInvariant());
			
			quark.Log ("[SendRequest()] " + url);
			
			var future = new Future<string>("SendRequest() + url");
			var bestType = Support.TypeToBestHTTPType(type);
			HTTPRequest request = new HTTPRequest(new System.Uri(url), bestType, OnFinished);
			//			request.ConnectTimeout = TimeSpan.FromSeconds(kConnectTimeoutSeconds);
			//			request.Timeout = TimeSpan.FromSeconds(kConnectTimeoutSeconds);
			request.DisableCache = true;
			request.Tag = future;
			
			if(p != null){
				foreach(var h in p._headers){
					request.AddHeader (h.Key, h.Value);
				}
				foreach(var h in p._params){
					request.AddField (h.Key, h.Value);
				}
				
				request.RawData = p._rawData;
			}
//			request.FieldCollector.LongLength = 8000;
			request.Send ();

			quark.Assert(CheckInvariant ());
			return future;
		}

		private static void OnFinished(HTTPRequest request, HTTPResponse response){
			quark.Assert (request != null);
//			quark.Assert (request.State != HTTPRequestStates.Aborted);
//			quark.Assert (request.State >= HTTPRequestStates.Finished);
			
			Future<string> future = (Future<string>)request.Tag;
			quark.Assert(future != null);
			
			GETResult result = Support.InterpretResponse(request);
			quark.Log ("[BestHTTP-reply] " + result._debugInfo + " [" + (response != null ? response.DataAsText : "") + "]");
			
			//	Glitch causes empty responses - send request again.
			if(result._status == Status.kMissingResponseDefect){
				request.Send ();
			}
			else{
				if(result._status == Status.kOK){
					try{
						future.Resolve(result._response);
					}
					catch{
						quark.Assert (false);
					}
				}
				else{
					try{
						future.Fail(Support.MakeException(result));
					}
					catch{
						quark.Assert (false);
					}
				}
			}
		}


}	//	REST





#if 0
public const string kParseBaseURL = "https://api.parse.com/";
public const string kParseTrolldomTest_ApplicationID = "bs9Ppq7DAYG05P5dejwiDSpDZK4eDDysU55njMDD";
public const string kParseTrolldomTest_RestAPIKey = "v3fQME4iV0V1QorDrNaAE9ITCZiGklV706mXVni2";
#endif



struct session_t {
	std::string _sessionToken;
	std::string _username;
	std::string _user_object_id;
};



//////////////////////////					SendRequest<T>

/*
	Lets you do REST requests to a Parse.com server via its REST-api, including credentialts for the server etc.
	Expects all replies to be JSON, directly mappable to data type T.
	Implement a data type T specifically for each reply.


	type = GET / PUT etc.
*/

//	p: optional.
//	Reply objects must be class, not struct. Future<> uses null _output to mean no output. Structs will be by-value = wrong.
public static Future<T> SendRequest<T>(REST.RESTManager rest, REST.Type type, ServerSpec server, UserSessionIm? userSession, string parseCommandURL, REST.RequestParams p){
	quark.Assert (rest.CheckInvariant());
	quark.Assert (server.CheckInvariant());
	quark.Assert (p == null || p.CheckInvariant());

//quark.Throw(quark.MakeArgumentException(kParseResponse, "TEST_38383", "param-xyz"));

	var result = new Future<T>("Parse_SendRequest<T>() " + parseCommandURL);
	string url = server._parseBaseURL + parseCommandURL;
	var parseParams = MakeParseParams(server, userSession, p);
	var future1 = rest.SendRequest(type, url, parseParams);
	future1.SetNotification(
		(f, error) => {
			if(error == null){
				string reply = f.GetValue();
				var unpacked = JsonMapper.ToObject<T>(reply);
				result.Resolve(unpacked);
			}
			else{
				result.Fail(error);
			}
		}
	);
	
	return result;
}

#endif






#if 0

	public static Future<GetBossDefsFromSHA1ReplyRaw> GETBossDefsFromSHA1s(REST.RESTManager rest, ServerSpec server, List<SHA1Hash> sha1s){
		quark.Assert (rest.CheckInvariant());
		quark.Assert (server.CheckInvariant());
		quark.Assert(sha1s != null);
		foreach(var s in sha1s){
			quark.Assert(s.CheckInvariant());
		}
		
		var sha1Strings = new List<string>();
		foreach(var i in sha1s){
			sha1Strings.Add (i.GetString());
		}

		string json = JsonMapper.ToJson(sha1Strings);
		string whereValue = "{\"sha1\":{\"$in\":" + json + "}}";

		//		string uri = "1/classes/BossDef/get?where=" + WWW.EscapeURL (whereValue); ???
		string uri = "1/classes/BossDef/?where=" + WWW.EscapeURL (whereValue);
		return SendRequest<GetBossDefsFromSHA1ReplyRaw>(rest, REST.Type.kGET, server, null, uri, null);
	}



	public class GetPublishedBossReplyRaw {
		public string objectId;
		public string createdAt;
		public string updatedAt;
		public string boss_def_sha1;
		public string name;
		public string description;

		//	Default is false if field is missing on Parse object.
		public bool listed;

		public UserRefRaw author = new UserRefRaw();
	}



	public class LoginReplyRaw {
		public string username;
		public string email;
		public string createdAt;
		public string updatedAt;
		public string objectId;
		public string sessionToken;
		public string draft_sha1;
	}

	public static Future<LoginReplyRaw> Login(REST.RESTManager rest, ServerSpec server, string username, string password){
		quark.Assert (rest.CheckInvariant());
		quark.Assert (server.CheckInvariant());
		quark.Assert (username.Length > 0);
		quark.Assert (password.Length > 0);
		
		var p = new REST.RequestParams();
		p._params.Add("username", username);
		p._params.Add("password", password);
		return SendRequest<LoginReplyRaw>(rest, REST.Type.kGET, server, null, "1/login", p);
	}

#endif

