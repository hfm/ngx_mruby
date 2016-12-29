method  = "";
user    = "";
passwd  = "";

r = Nginx::Request.new
r.headers_in.all.each_pair do |k, v|

  case k.downcase
  when 'auth-method'
    method = v
  when 'auth-user'
    user = v
  when 'auth-pass'
    passwd = v
  end

  if k == "auth-method"
    method = v;
  elsif k == "auth-user"
    user   = v;
  elsif k == "auth-pass"
    passwd = v;
  end
end

h = Nginx::Headers_out.new

if method == "plain" and user == "username" and passwd == "password" then
  h["Auth-Status"] = "OK"
  h["Auth-Server"] = "127.0.0.1"
  h["Auth-Port"]   = "25"
  h["Auth-User"]   = ""
  h["Auth-Pass"]   = ""
  h["Auth-Method"]   = "none"
else
  h["Auth-Status"] = "Invalid login or password"
  h["Auth-Wait"]   = "3"
end
