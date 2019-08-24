# Kleines Arduino-Projekt zum Steuern einer Vakuumkammer


  Die Vakuumkammer besteht neben der Kammer selbst, aus einer Vakuumpumpe, zwei Ventilen und einem Drucksensor. Der Arduino steuert die Pumpe und die Ventile über Relais und misst den Druck in der Kammer über einen Drucksensor.

  Ziel ist, in der Druckkammer Steig- und Sinkflüge zyklisch zu simulieren um Vergaserschwimmer auf Ihre Drucktoleranz hin zu testen.

# Setup des Arduino-Projekts
  * Boardtype: Arduino Nano
  * Display:   KUMAN 3.5"TFT LCD Shield (ILI9481 Controller)

# Arduino IDE einrichten
  * Arduino-IDE installieren _https://www.arduino.cc/en/Main/Software_
  * schwimmertester.ino Sketch öffnen (Verzeichnis _Src/schwimmertester_)
  * Damit der Sketch kompiliert werden kann, müssen einige Bibliotheken in die Toolchain eingebunden werden. Alle benötigten Bibliotheken liegen im Ordner **Libs**. Um die Bibliotheken einzubinden, in der Arduino-IDE wie folgt vorgehen:
    Menu Sketch->Bibliothek einbinden->Zip-Bibliothek hinzufügen
    Diesen Befehl für jede Zip-Datei ausführen, die sich im Verzeichnis _Libs_ befindet.
  * Kompilieren des Codes mit Strg-R. 

#Autoren
  * Thorsten Klinkhammer
  * Markus Kuhnla