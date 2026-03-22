const zigbeeHerdsmanConverters = require('zigbee-herdsman-converters');
const zigbeeHerdsmanUtils = require('zigbee-herdsman-converters/lib/utils');


const exposes = zigbeeHerdsmanConverters['exposes'] || require("zigbee-herdsman-converters/lib/exposes");
const ea = exposes.access;
const e = exposes.presets;
const modernExposes = (e.hasOwnProperty('illuminance_lux'))? false: true;

const fz = zigbeeHerdsmanConverters.fromZigbeeConverters || zigbeeHerdsmanConverters.fromZigbee;
const tz = zigbeeHerdsmanConverters.toZigbeeConverters || zigbeeHerdsmanConverters.toZigbee;

const distance = {
    cluster: 'genAnalogInput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
        if (msg.data.hasOwnProperty('presentValue')) {
            const value = msg.data.presentValue;
            return {
                distance: Math.round(value) // float -> int
            };
        }
    },
};

const device = {
    zigbeeModel: ['distance-sensor'],
    model: 'distance-sensor',
    vendor: 'xzkostyan',
    description: 'VL53L1X distance sensor',
    fromZigbee: [distance],
    toZigbee: [],
    exposes: [
	e.numeric('distance', exposes.access.STATE)
	    .withLabel("Расстояние")
            .withUnit('mm')
            .withDescription('Расстояние до объекта')
    ]
};

module.exports = device;
