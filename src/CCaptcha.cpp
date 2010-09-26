#include "CCaptcha.h"
#include "CDaemon.h"

CCaptcha::CCaptcha(const QByteArray & data, QObject * parent) :
	QObject(parent)
{
	m_data.data = data;
	m_data.captcha = this;

	for(int i = 0; i < 4; ++i)
		m_spikeCount[i].fill(0, 10);
}

void CCaptcha::run()
{
	{
		struct mad_decoder decoder;

		mad_decoder_init(&decoder, &m_data, CCaptcha::input, 0 , 0 , CCaptcha::output, 0, 0);
		int result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
		mad_decoder_finish(&decoder);

		if(result != 0 || m_data.abort)
		{
			emit finished(false, QString());
			return;
		}
	}

	quint32 minDiff[4] = { 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF };
	QString captcha("----");

	for(QList<CCaptchaLetter>::const_iterator it = CDaemon::captchaLetters.begin(); it != CDaemon::captchaLetters.end(); ++it)
	{
		for(quint8 i = 0; i < 4; ++i)
		{
			quint32 absDiff = it->absDiff(m_spikeCount[i].toList());
			if(absDiff < minDiff[i])
			{
				minDiff[i] = absDiff;
				captcha[i] = it->letter();
			}
		}
	}

	emit finished(!m_data.abort, captcha);
}

void CCaptcha::abort()
{
	m_data.abort = true;
}

void CCaptcha::analyzeSample(qint32 sample)
{
	if(sample == 0)
		return;

	if(sample >= 120)
		m_spikeCount[m_data.letterIndex][0]++;
	else if(sample >= 96)
		m_spikeCount[m_data.letterIndex][1]++;
	else if(sample >= 64)
		m_spikeCount[m_data.letterIndex][2]++;
	else if(sample >= 32)
		m_spikeCount[m_data.letterIndex][3]++;
	else if(sample >= 16)
		m_spikeCount[m_data.letterIndex][4]++;
	else if(sample <= -120)
		m_spikeCount[m_data.letterIndex][9]++;
	else if(sample <= -96)
		m_spikeCount[m_data.letterIndex][8]++;
	else if(sample <= -64)
		m_spikeCount[m_data.letterIndex][7]++;
	else if(sample <= -32)
		m_spikeCount[m_data.letterIndex][6]++;
	else if(sample <= -16)
		m_spikeCount[m_data.letterIndex][5]++;
}

mad_flow CCaptcha::input(void * data, mad_stream * stream)
{
	CaptchaInternalData * d = reinterpret_cast<CaptchaInternalData *>(data);
	if(d->input)
		return MAD_FLOW_STOP;

	mad_stream_buffer(stream, (const unsigned char *)d->data.data(), d->data.size());
	d->input = true;
	return MAD_FLOW_CONTINUE;
}

mad_flow CCaptcha::output(void * data, const mad_header * header, mad_pcm * pcm)
{
	CaptchaInternalData * d = reinterpret_cast<CaptchaInternalData *>(data);

	quint32 samplesCount;
	mad_fixed_t const * left_ch;

	samplesCount = pcm->length;
	left_ch = pcm->samples[0];

	while(--samplesCount != 0)
	{
		qint32 sample = *left_ch++;
		sample += (1L << (MAD_F_FRACBITS - 16));
		if (sample >= MAD_F_ONE)
			sample = MAD_F_ONE - 1;
		else if (sample < -MAD_F_ONE)
			sample = -MAD_F_ONE;
		sample >>= (MAD_F_FRACBITS + 1 - 16);
		sample /= 256;

		if(sample == 0)
		{
			d->zeroCount++;
		}
		else
		{
			d->zeroCount = 0;
			d->space = false;
		}

		if(d->zeroCount <= 150)
		{
			if(d->letterIndex >= 0 && d->letterIndex < 4)
				d->captcha->analyzeSample(sample);
		}
		else if(!d->space)
		{
			d->space = true;
			if(d->letterIndex == 3)
				return MAD_FLOW_STOP;
			d->letterIndex++;
		}
	}

	return MAD_FLOW_CONTINUE;
}
