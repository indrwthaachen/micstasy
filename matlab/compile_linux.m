disp('compiling micstasy interface for matlab... ') 
mex micstasy.c ../micstasyc.c -lportmidi 
if exist('micstasy') 
disp('micstasy matlab interface compiled successfully !')
end
