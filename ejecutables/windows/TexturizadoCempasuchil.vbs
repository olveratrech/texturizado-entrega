Option Explicit
Dim shell, fso, carpeta, comando, codigo
Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
carpeta = fso.GetParentFolderName(WScript.ScriptFullName)
comando = "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File """ & carpeta & "\iniciar_windows.ps1""""
codigo = shell.Run(comando, 0, True)
If codigo <> 0 Then
  MsgBox "No se pudo compilar o ejecutar Texturizado Cempasúchil. Revisa la carpeta logs.", 16, "Texturizado Cempasúchil"
End If
