delete(instrfind({'Port'},{'COM4'}))
nano=serialport('COM4',115200);

configureTerminator(nano, "LF");

% Create a 6-element uint8 array
data =("001002003004005006");

% Send the data to Arduino
writeline(nano, data);

clear nano;