using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Net.Sockets;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;

namespace HttpsSocketProxy {
	class MainClass {
		public static readonly Dictionary<string, int> DefaultPorts;
		public static Uri ProxyHost;
		public static Uri TargetHost;
		public static string PasswordFile;

		static MainClass() {
			DefaultPorts = new Dictionary<string, int>();
			DefaultPorts["ssh"] = 22;
			DefaultPorts["telnet"] = 23;
			DefaultPorts["http"] = 80;
			DefaultPorts["https"] = 443;
		}

		private static void ParseArguments(string[] args) {
			if ( args.Length < 2 ) {
				throw new ArgumentException("Too few arguments");
			}
			if ( args.Length > 3 ) {
				throw new ArgumentException("Too many arguments");
			}
			ProxyHost = new Uri(args[0]);
			if ( ProxyHost.Scheme != Uri.UriSchemeHttp && ProxyHost.Scheme != Uri.UriSchemeHttps ) {
				throw new ArgumentException(string.Format("Invalid proxy scheme: {0}", ProxyHost.Scheme));
			}
			TargetHost = new Uri(args[1]);
			PasswordFile = args.Length == 3 ? args[2] : null;
		}

		private static bool VerifyRemoteCert(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors) {
			return true;
		}

		private static Stream CreateProxyConnection(TcpClient client) {
			if ( ProxyHost.Scheme == Uri.UriSchemeHttp ) {
				return client.GetStream();
			} else {
				SslStream ssl = new SslStream(client.GetStream(), false, VerifyRemoteCert);
				ssl.AuthenticateAsClient(ProxyHost.Host);
				return ssl;
			}
		}

		private static bool TryConnect(string auth) {
			using ( TcpClient client = new TcpClient(ProxyHost.Host, ProxyHost.Port) ) {
				using ( Stream stream = CreateProxyConnection(client) ) {
					using ( StreamWriter writer = new StreamWriter(stream, Encoding.ASCII, 1024, true) ) {
						writer.NewLine = "\r\n";
#if DEBUG
						Console.Error.WriteLine("CONNECT {0}:{1} HTTP/1.1", TargetHost.Host, TargetHost.IsDefaultPort ? (DefaultPorts.ContainsKey(TargetHost.Scheme) ? DefaultPorts[TargetHost.Scheme] : 23) : TargetHost.Port);
						if ( auth != null ) {
							Console.Error.WriteLine("Proxy-Authorization: Basic {0}", Convert.ToBase64String(Encoding.ASCII.GetBytes(auth)));
						}
						Console.Error.WriteLine("Host: {0}{1}{2}", ProxyHost.Host, ProxyHost.IsDefaultPort ? "" : ":", ProxyHost.Port);
						Console.Error.WriteLine("User-Agent: HTTPS Socket Proxy (https://github.com/zachdeibert/https-socket-proxy/)");
						Console.Error.WriteLine("Accept: */*");
						Console.Error.WriteLine();
#endif
						writer.WriteLine("CONNECT {0}:{1} HTTP/1.1", TargetHost.Host, TargetHost.IsDefaultPort ? (DefaultPorts.ContainsKey(TargetHost.Scheme) ? DefaultPorts[TargetHost.Scheme] : 23) : TargetHost.Port);
						if ( auth != null ) {
							writer.WriteLine("Proxy-Authorization: Basic {0}", Convert.ToBase64String(Encoding.ASCII.GetBytes(auth)));
						}
						writer.WriteLine("Host: {0}{1}{2}", ProxyHost.Host, ProxyHost.IsDefaultPort ? "" : ":", ProxyHost.Port);
						writer.WriteLine("User-Agent: HTTPS Socket Proxy (https://github.com/zachdeibert/https-socket-proxy/)");
						writer.WriteLine("Accept: */*");
						writer.WriteLine();
					}
					using ( StreamReader reader = new StreamReader(stream, Encoding.ASCII, false, 1024, true) ) {
						string statusResp = reader.ReadLine();
#if DEBUG
						Console.Error.WriteLine(statusResp);
#endif	
						string[] parts = statusResp.Trim('\r', '\n').Split(' ');
						if ( parts.Length < 2 || parts[1] != "200" ) {
							return false;
						}
#pragma warning disable 219
						string tmp;
#pragma warning restore 219
						while ( (tmp = reader.ReadLine()).Trim('\r', '\n').Length > 0 ) {
#if DEBUG
							Console.Error.WriteLine(tmp);
#endif	
						}
					}
					Task outputTask = stream.CopyToAsync(Console.OpenStandardOutput());
					Task inputTask = Console.OpenStandardInput().CopyToAsync(stream);
					outputTask.Wait();
					inputTask.Wait();
					return true;
				}
			}
		}

		public static void Main(string[] args) {
			try {
				ParseArguments(args);
			} catch ( ArgumentException ex ) {
				Console.Error.WriteLine(ex.Message);
				Console.Error.WriteLine("Usage: httpssocketproxy.exe <proxy host> <target host> [password file]");
				Console.Error.WriteLine();
				Console.Error.WriteLine("proxy host:    The host to connect to for the proxying (hostname:port)");
				Console.Error.WriteLine("target host:   The host to HTTP CONNECT to from the proxy host (hostname:port)");
				Console.Error.WriteLine("password file: The file containing authentication credentials for the proxy (username:password on each line)");
				Console.Error.WriteLine();
				Console.Error.WriteLine("Version: {0}", Assembly.GetEntryAssembly().GetName().Version);
				return;
			}
			bool success = false;
			if ( PasswordFile == null ) {
				success = TryConnect(null);
			} else {
				using ( Stream stream = File.Open(PasswordFile, FileMode.Open, FileAccess.Read) ) {
					using ( StreamReader reader = new StreamReader(stream) ) {
						while ( !reader.EndOfStream ) {
							if ( (success = TryConnect(reader.ReadLine())) ) {
								break;
							}
						}
					}
				}
			}
			if ( !success ) {
				Console.Error.WriteLine("Unable to connect using any of the specified credentials");
			}
		}
	}
}
