# Lille Bus/Tramway timetable for ESP32

![Running](https://imgur.com/njUgrnd)


Uses the MEL's implementation of the opendata API, documentation [here](https://help.opendatasoft.com/apis/ods-search-v1/#search-api-v1).

To refresh the timer every minute (more than 1000 times a day) you need an API key, available after creating an account [here](https://opendata.lillemetropole.fr/signup/).

To modify the code for your use case, modify the request in the `url` String. You can use this [online tool](https://opendata.lillemetropole.fr/explore/dataset/ilevia-prochainspassages/api/) to find this easily.

