portmidipath = input('please enter the path of the folder that contains portmidi.h : ', 's') 

disp('compiling micstasy interface for matlab... ') 
mexcmd = strcat('mex -g micstasy.c ../micstasyc.c -I', portmidipath, ' -L. -lportmidi');
eval(mexcmd)
if exist('micstasy') 
disp('micstasy matlab interface compiled successfully !')
end
