/*
 * Copyright (C) 2017 Jannik Beyerstedt <jannik.beyerstedt@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

package main

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"log"
	"os"

	"github.com/zubairhamed/canopus"
)

func main() {
	server := canopus.NewServer()

	server.Get("/hello", func(req canopus.Request) canopus.Response {
		log.Println("Hello Called")
		msg := canopus.ContentMessage(req.GetMessage().GetMessageId(), canopus.MessageAcknowledgment)
		msg.SetStringPayload("hello")

		res := canopus.NewResponse(msg, nil)
		return res
	})

	server.Get("/update", func(req canopus.Request) canopus.Response {
		// get parameters from requests
		reqBytes := req.GetMessage().GetPayload().GetBytes()
		var fwVers []byte = reqBytes[0:2]
		var slotNo byte = reqBytes[2]
		var hwId []byte = reqBytes[3:11]

		// construct filename
		filename := "fw_update-0x"
		filename += hex.EncodeToString(hwId)
		var versInt uint16
		binary.Read(bytes.NewReader(fwVers), binary.BigEndian, &versInt)
		filename += "-0x" + fmt.Sprintf("%d", versInt+1)
		filename += "-s" + fmt.Sprintf("%d", slotNo)
		filename += ".bin"

		log.Println("Update Requested for:", filename)

		// search for the file
		var msg canopus.Message

		if _, err := os.Stat("./" + filename); err == nil {
			log.Println("file available, sending answer with filename")

			// construct answer (success (2.05), filename as payload)
			msg = canopus.ContentMessage(req.GetMessage().GetMessageId(), canopus.MessageAcknowledgment)
			msg.SetPayload(canopus.NewPlainTextPayload(filename))
		} else {
			log.Println("file does not exist, sending empty answer")

			// construct answer (success (2.05), not payload)
			msg = canopus.ContentMessage(req.GetMessage().GetMessageId(), canopus.MessageAcknowledgment)
		}

		res := canopus.NewResponse(msg, nil)
		return res
	})

	// TODO: add file download using CoAP block transfer

	server.ListenAndServe(":5683")
	<-make(chan struct{})
}
