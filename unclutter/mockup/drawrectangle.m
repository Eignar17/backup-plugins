function out = drawrectangle(R,color)
### A rectangle is a 2x2 matrix where the first column gives the x
### coordinates and the second column gives the y coordinate

  x = [R(1,1) R(2,1) R(2,1) R(1,1) R(1,1)]'; 
  y = [R(1,2) R(1,2) R(2,2) R(2,2) R(1,2)]'; 
  
  out = fill(x,y,color );
endfunction