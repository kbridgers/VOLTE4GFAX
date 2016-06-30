<html>
<head>
<title>LOGIN</title>
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<link rel="stylesheet" type="text/css" href="final.css">
<SCRIPT language="JavaScript">
if (top.location != self.location)
    top.location=self.location;
function evaltF()
{
  var user = document.tF.usr.value;
  var passwd = document.tF.pws.value;
/*
  if((user != "admin") && (user != "root"))
  {
    alert("Invalid Username");
    return false;
  }
*/
  return true;
}
</script>

</head>
<body class="decBackgroundDef" onLoad="document.tF.usr.focus();">
<table width="100%" border="0" cellspacing="0" cellpadding="0" height="80%">
  <tr>
	<td valign="middle"> 
	  <form action="/goform/ifx_set_login" method="post" name="tF">
	  <input type="hidden" name="page" value="frame_setup.htm">
		<div align="center">
		<table border="0" cellspacing="1" cellpadding="5" bgcolor="#336699" width="250">
		  <tr>
			<th>FLTE LOGIN - FWBOOT</th>
		  </tr>
		  <tr>
		    <td height="3" bgcolor="#ffffff"> 
			<table border="0" cellspacing="0" cellpadding="5">
        <tr class="decBold">
          <td>Username:</td>
          <td><input type="text" maxLength=16 size=20 name="usr"></td>
        <tr> 
			  <tr class="decBold"> 
			    <td>Password:</td>
			    <td><input type="password" maxLength=16 size=20 name="pws"></td>
			  </tr>
			  <tr align="right"> 
			    <td><input type="submit" value="" style="position:absolute;Z-index:-1;height:0;width:0" onClick="return evaltF();"><a class="button" href="javascript:document.forms[0].submit()" value="LOGIN" onClick="return evaltF();">LOGIN</a></td>
          <td><a class="button" href="javascript:document.tF.reset()">CANCEL</a></td>
			</tr>
		    </table>
		</td>
		  </tr>
		</table>
		</div>
	  </form>
	</td>
  </tr>
</table>
</body>
</html>

