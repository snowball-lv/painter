Paints a black and white [.ppm](https://en.wikipedia.org/wiki/Netpbm#PPM_example)
file and prints the number of contiguous shapes.  
Check command line for help.

Use either [`convert`](https://imagemagick.org/script/convert.php) or a
[plugin](https://marketplace.visualstudio.com/items?itemName=ngtystr.ppm-pgm-viewer-for-vscode)
to preview `.ppm` files.

1. `./bin/paint -g 16 in.ppm`  
<img src="./in.png" width=200 style="image-rendering: pixelated">
2. `./bin/paint in.ppm out.ppm`  
<img src="./out.png" width=200 style="image-rendering: pixelated">
```bash
shapes: 25
```
## Build

```bash
make
```
