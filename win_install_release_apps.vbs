' Clean and copy SyntroCore apps to the %SYNTRODIR%/bin directory.
'
' The script expects SYNTRODIR to be in the environment and for the
' %SYNTRODIR%/bin directory to already exist.
'
' If it does not already exists, this indicates that the Syntro libraries
' are not installed and the apps will not work anyway.
'

Set fso = CreateObject("Scripting.FileSystemObject")

Set ws = CreateObject("WScript.Shell")

SyntroDir = ws.ExpandEnvironmentStrings("%SYNTRODIR%")

If Not fso.FolderExists(SyntroDir) Then
    MsgBox "Environment variable SYNTRODIR is not defined", vbOkOnly, "Fatal Error"
    WScript.Quit
End If

On Error Resume Next

CopyApps


Sub CopyApps()

	dstFolder = SyntroDir + "/bin/"

	srcFile = SyntroDir + "/SyntroCore/SyntroCFS/Release/SyntroCFS.exe"
	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If

	srcFile = SyntroDir + "/SyntroCore/SyntroControl/Release/SyntroControl.exe"
	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If

	srcFile = SyntroDir + "/SyntroCore/SyntroExec/Release/SyntroExec.exe"
	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If

	srcFile = SyntroDir + "/SyntroCore/SyntroLog/Release/SyntroLog.exe"
	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If

	srcFile = SyntroDir + "/SyntroCore/SyntroStore/Release/SyntroStore.exe"
	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If

End Sub
