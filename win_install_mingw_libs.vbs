' Clean and copy SyntroGUI/SyntroLib files to the %SYNTRODIR% include 
' and bin directories.
'
' The script expects SYNTRODIR to be in the environment.
'
' The folders bin and include will be created if they don't exist
'

Set fso = CreateObject("Scripting.FileSystemObject")

Set ws = CreateObject("WScript.Shell")

SyntroDir = ws.ExpandEnvironmentStrings("%SYNTRODIR%")

If Not fso.FolderExists(SyntroDir) Then
    MsgBox "Environment variable SYNTRODIR is not defined", vbOkOnly, "Fatal Error"
    WScript.Quit
End If

On Error Resume Next

CleanAll
CopyHeaders
CopyLibs
CopyDLLs


Sub CleanAll

	folderName = SyntroDir + "/include"
    
	If Not fso.FolderExists(folderName) Then
		fso.CreateFolder folderName
	Else
		fso.DeleteFile folderName + "/*.h"
	End If

	folderName = SyntroDir + "/include/SyntroControlLib"

	If Not fso.FolderExists(folderName) Then
		fso.CreateFolder folderName
	Else
		fso.DeleteFile folderName + "/*.h"
	End If

	folderName = SyntroDir + "/include/SyntroAV"

	If Not fso.FolderExists(folderName) Then
		fso.CreateFolder folderName
	Else
		fso.DeleteFile folderName + "/*.h"
	End If

	folderName = SyntroDir + "/bin"
	   
	If Not fso.FolderExists(folderName) Then
		fso.CreateFolder folderName
	Else
		fso.DeleteFile folderName + "/*.dll"
	End If

End Sub


Sub CopyHeaders

	dstFolder = SyntroDir + "/include/"

	srcFolder = SyntroDir + "/SyntroCore/SyntroLib"

	If Not fso.FolderExists(srcFolder) Then
		MsgBox srcFolder + " not found", vbOkOnly, "Error"
		Exit Sub
	End If

	Set src = fso.getfolder(srcFolder)

	For Each file in src.Files
		If fso.GetExtensionName(file) = "h" Then
			file.Copy dstFolder
		End If
	Next

	dstFolder = SyntroDir + "/include/SyntroControlLib/"
	srcFolder = SyntroDir + "/SyntroCore/SyntroControlLib"

	If Not fso.FolderExists(srcFolder) Then
		MsgBox srcFolder + " not found", vbOkOnly, "Error"
		Exit Sub
	End If

	Set src = fso.getfolder(srcFolder)

	For Each file in src.Files
		If fso.GetExtensionName(file) = "h" Then
			file.Copy dstFolder
		End If
	Next

	srcFolder = SyntroDir + "/SyntroCore/SyntroGUI"

	If Not fso.FolderExists(srcFolder) Then
		MsgBox srcFolder + " not found", vbOkOnly, "Error"
		Exit Sub
	End If

	Set src = fso.getfolder(srcFolder)

	For Each file in src.Files
		If fso.GetExtensionName(file) = "h" Then
			file.Copy dstFolder
		End If
	Next


	dstFolder = SyntroDir + "/include/SyntroAV/"

	srcFolder = SyntroDir + "/SyntroCore/SyntroLib/SyntroAV"

	If Not fso.FolderExists(srcFolder) Then
		MsgBox srcFolder + " not found", vbOkOnly, "Error"
		Exit Sub
	End If

	Set src = fso.getfolder(srcFolder)

	For Each file in src.Files
		If fso.GetExtensionName(file) = "h" Then
			file.Copy dstFolder
		End If
	Next



End Sub


Sub CopyDLLs()

	dstFolder = SyntroDir + "/bin/"

	srcFile = SyntroDir + "/SyntroCore/SyntroLib/Release/SyntroLib.dll"

	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If


	srcFile = SyntroDir + "/SyntroCore/SyntroControlLib/Release/SyntroControlLib.dll"

	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If

	srcFile = SyntroDir + "/SyntroCore/SyntroGUI/Release/SyntroGUI.dll"

	If Not fso.FileExists(srcFile) Then
		MsgBox srcFile + " not found", vbOkOnly, "Copy Error"
		Exit Sub
	Else
	    fso.CopyFile srcFile, dstFolder
	End If

End Sub
