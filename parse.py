import re, json
f=open("usb.ids", "r",errors="replace")

vendor_name=""
vendor_id=""

result=[]

tab="\t"
for i, line in enumerate(f.readlines()):
    line=line.removesuffix("\n")

    if "List of known device classes, subclasses and protocols" in line:
        break

    if line.startswith("#") or (line==""):
        continue
    
    span=re.match(rf"^({tab})*[{tab}]*",line).span()
    
    num_tabs=span[1]-span[0]

    if num_tabs==0:
        vendor_id, vendor_name = line.split(None,1)
        result.append([vendor_name,[int(vendor_id,16), -1]])
    elif num_tabs==1:
        product_id, product_name = line.split(None,1)
        result.append([vendor_name+" "+product_name,[int(vendor_id,16), int(product_id,16)]])
    else:
        pass

f.close()

f=open("usb.ids.json", "w+")
json.dump(result, f)

f.close()

subprocess.run(["xxd", "-i", "usb.ids.json", "usb.ids.h"])
