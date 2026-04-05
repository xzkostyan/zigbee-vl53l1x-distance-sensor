const zigbeeHerdsmanConverters = require('zigbee-herdsman-converters');
const zigbeeHerdsmanUtils = require('zigbee-herdsman-converters/lib/utils');
const reporting = require('zigbee-herdsman-converters/lib/reporting');


const exposes = zigbeeHerdsmanConverters['exposes'] || require("zigbee-herdsman-converters/lib/exposes");
const ea = exposes.access;
const e = exposes.presets;
const modernExposes = (e.hasOwnProperty('illuminance_lux'))? false: true;

const fz = zigbeeHerdsmanConverters.fromZigbeeConverters || zigbeeHerdsmanConverters.fromZigbee;
const tz = zigbeeHerdsmanConverters.toZigbeeConverters || zigbeeHerdsmanConverters.toZigbee;

const ATTRID_READING_INTERVAL = 0x0201;
const ZCL_DATATYPE_UINT16 = 0x21;

const fzDistance = {
    cluster: 'genAnalogInput',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
	const result = {};

	if (msg.data.hasOwnProperty(ATTRID_READING_INTERVAL)) {
            result.reading_interval = msg.data[ATTRID_READING_INTERVAL];
        }

        if (msg.data.hasOwnProperty('presentValue')) {
            const value = msg.data.presentValue;
            result.distance = Math.round(value); // float -> int
        }

	return result;
    },
};

const tzDistance = {
    key: ['reading_interval'],
    convertSet: async (entity, key, rawValue, meta) => {
        const value = parseInt(rawValue, 10);
	const readingIntervalPayload = {};
	readingIntervalPayload[ATTRID_READING_INTERVAL] = {value, type: ZCL_DATATYPE_UINT16};
        const payloads = {
            reading_interval: ['genAnalogInput', readingIntervalPayload]
        };
        await entity.write(payloads[key][0], payloads[key][1]);
        return {
            state: {[key]: rawValue},
        };
    }
};

const device = {
    zigbeeModel: ['distance-sensor'],
    model: 'distance-sensor',
    vendor: 'xzkostyan',
    description: 'VL53L1X distance sensor',
    fromZigbee: [fzDistance],
    toZigbee: [tzDistance],
    exposes: [
	e.numeric('distance', exposes.access.STATE)
	    .withLabel('Расстояние')
            .withUnit('mm')
            .withDescription('Расстояние до объекта'),
        e.numeric('reading_interval', ea.STATE_SET)
	    .withLabel('Интервал')
	    .withUnit('Seconds')
	    .withDescription('Интервал срабатывания сенсора, по умолчанию 60 сек')
	    .withValueMin(5)
	    .withValueMax(3600)
    ],
    configure: async (device, coordinatorEndpoint) => {
        const endpoint = device.getEndpoint(1);
        await reporting.bind(endpoint, coordinatorEndpoint, ["genAnalogInput"]);
        const payload = reporting.payload("presentValue", 60, 3600, 0);
        await endpoint.configureReporting("genAnalogInput", payload);
        await endpoint.read("genAnalogInput", ["presentValue"]);
    }
};

module.exports = device;
