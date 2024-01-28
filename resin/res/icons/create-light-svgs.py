# go through all svgs in the current folder that don't end in -light.svg
import os, re

for filename in os.listdir("."):
    if filename.endswith(".svg") and not filename.endswith("-light.svg"):
        # get the name of the file without the extension
        name = os.path.splitext(filename)[0]
        with open(filename, "r") as svg:
            # read the file
            data = svg.read()
            # replace the fill color
            data = re.sub(r'<svg ', '<svg fill="#FFFFFF" ', data)
            # write the file
            with open(name + "-light.svg", "w") as svg_out:
                svg_out.write(data)