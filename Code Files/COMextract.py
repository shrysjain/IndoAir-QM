import code
from xml.etree.ElementTree import tostring
import serial
import time
from discord_webhook import DiscordWebhook, DiscordEmbed

ser = serial.Serial('COM3',115200)
time.sleep(2);

while True:
    
    line = ser.readline()
    if line:
        string = line.decode()

        # Value "string" is the now the AQissue

        if string == "No Issue with Air Quality":
            issueFix = "N/A"
            isIssue = False
        elif string == "Too Cold and Too Humid":
            issueFix = "Use a heater or air conditioner to heaten the area and bring down the humidity"
            isIssue = True
        elif string == "Too Cold":
            issueFix = "Use a heater or turn off AC/fans/anything cooling the area"
            isIssue = True
        elif string == "Too Hot and Too Humid":
            issueFix = "Use an air conditioner to dry out the air and circulate cold air"
            isIssue = True
        elif string.startswith("Too Hot"):
            issueFix = "Use AC/fans to circulate more cold air"
            isIssue = True
        elif string == "Too Humid":
            issueFix = "Run a heater or air conditioner which will dry out the air in your area"
            isIssue = True
        elif string == "Not Humid Enough":
            issueFix = "Use a vaporizer, steam machine, or humidifier to raise humidity and moisture"
            isIssue = True
        elif string == "Too Cold and Not Humid Enough":
            issueFix = "Use humidifers/heaters or try to tackle the issues one-by-one"
            isIssue = True
        else:
            issueFix = "N/A"
            isIssue = False

        print(string)

        if isIssue == True:
            webhook = DiscordWebhook("Discord Webhook URL", content="There is an issue with the indoor air quality! See the embed attached to this message for more information ASAP.")
            embed = DiscordEmbed(title="Issue with Indoor Air Quality", description="There is an issue with the indoor air quality! Here is the information:\n\n**ISSUE:** " + string + "\n**SOLUTION TO FIX ISSUE: **" + issueFix + "\n\nTry to solve this issue quickly before the situation gets worse. If you are not able to implement the solution recommended above, refer to [this](https://www.servicefirstprosllc.com/expert-tips/ways-to-raise-or-lower-your-homes-humidity/) website for information on adjusting humidity, and try to do the solutions that will fix both the bad humidity levels and the bad temperature levels (if applicable).", color="00bcff")
            webhook.add_embed(embed)
            response = webhook.execute()
            print(response)

        string = line.decode()

ser.close()
